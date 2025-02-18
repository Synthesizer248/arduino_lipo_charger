#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Adafruit_INA219.h> // Library for INA219 current sensor

// Pin Definitions
#define BUZZER_PIN 7
#define LOAD_PIN 8
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_SW 4
#define LM2596_PWM_PIN 9

// Watchdog Timer
#include <avr/wdt.h>

// INA219 Setup
Adafruit_INA219 ina219;

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
float maxChargingVoltage = 4.2; // Maximum safe charging voltage for LiPo
float minChargingVoltage = 3.0; // Minimum safe charging voltage
float nominalChargingVoltage = 3.7;
int ratedCapacitymAh = 1000;
int typicalCapacitymAh = 1200;
float chargeCurrent = 0.0; // Measured charging current in Amps
float targetChargeCurrent = 0.5; // Target charging current for CC mode
float batteryVoltage = 0.0; // Measured battery voltage
float openCircuitVoltage = 0.0;
float loadedVoltage = 0.0;
float internalResistance = 0.0;
float loadCurrent = 0.0;
float loadResistance = 1.0;

int menuState = 0;
int encoderValue = 0;
int lastEncoderValue = 0;

// Safety Thresholds
const float MAX_SAFE_CURRENT = 2.0; // Maximum safe charging current (in Amps)
const float MAX_BATTERY_VOLTAGE = 4.2; // Maximum safe battery voltage (in Volts)

bool isCharging = false; // Tracks whether the system is currently charging
int pwmDutyCycle = 0; // Tracks the current PWM duty cycle

void setup() {
  // Initialize Serial Communication for Debugging
  //Serial.begin(9600);

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);
  digitalWrite(LOAD_PIN, LOW);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(LM2596_PWM_PIN, OUTPUT);

  // Initialize LCD Display
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("=Synthesizer248=");
  lcd.clear();
  lcd.print("LiPo Charger v0.1");
  delay(2000);
  lcd.clear();

  // Initialize INA219
  if (!ina219.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INA219 Error!");
    tone(BUZZER_PIN, 500, 500); // Alert with buzzer
    while (1); // Halt if INA219 fails to initialize
  }

  // Enable Watchdog Timer
  wdt_enable(WDTO_2S); // Reset after 2 seconds if the program hangs

  // Soft-Start Initialization
  analogWrite(LM2596_PWM_PIN, 0); // Start with 0% duty cycle
}

void loop() {
  wdt_reset(); // Reset the watchdog timer

  // Read Rotary Encoder
  int clkState = digitalRead(ENCODER_CLK);
  int dtState = digitalRead(ENCODER_DT);

  if (clkState != lastEncoderValue) {
    if (dtState != clkState) {
      encoderValue++;
    } else {
      encoderValue--;
    }
    tone(BUZZER_PIN, 1000, 50); // Audible feedback for encoder rotation
  }
  lastEncoderValue = clkState;

  // Handle Menu Navigation
  handleMenu();

  // Update Display
  updateDisplay();

  // Charging Logic
  if (menuState == 0) {
    softStart(); // Gradually increase voltage and current
    chargeBattery();
  }

  delay(100);
}

void softStart() {
  static bool softStartComplete = false;
  if (!softStartComplete) {
    for (pwmDutyCycle = 0; pwmDutyCycle <= 255; pwmDutyCycle += 5) {
      analogWrite(LM2596_PWM_PIN, pwmDutyCycle);
      delay(50); // Gradual increase over time
    }
    softStartComplete = true;
  }
}

void chargeBattery() {
  // Read Battery Voltage and Current from INA219
  batteryVoltage = ina219.getBusVoltage_V();
  chargeCurrent = ina219.getCurrent_mA() / 1000.0;

  // Check Safety Thresholds
  if (chargeCurrent > MAX_SAFE_CURRENT || batteryVoltage > MAX_BATTERY_VOLTAGE) {
    stopCharging();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error: Overload!");
    lcd.setCursor(0, 1);
    lcd.print("Resetting...");
    tone(BUZZER_PIN, 1000, 500); // Alert with buzzer
    delay(2000);
    wdt_enable(WDTO_15MS); // Force reset
    while (1); // Wait for reset
  }

  // CC/CV Control Logic
  if (batteryVoltage < maxChargingVoltage) {
    // Constant Current (CC) Mode
    if (chargeCurrent < targetChargeCurrent) {
      pwmDutyCycle = constrain(pwmDutyCycle + 1, 0, 255); // Increase PWM duty cycle smoothly
    } else if (chargeCurrent > targetChargeCurrent) {
      pwmDutyCycle = constrain(pwmDutyCycle - 1, 0, 255); // Decrease PWM duty cycle smoothly
    }
  } else {
    // Constant Voltage (CV) Mode
    if (batteryVoltage > maxChargingVoltage) {
      pwmDutyCycle = constrain(pwmDutyCycle - 1, 0, 255); // Decrease PWM duty cycle smoothly
    } else if (batteryVoltage < maxChargingVoltage) {
      pwmDutyCycle = constrain(pwmDutyCycle + 1, 0, 255); // Increase PWM duty cycle smoothly
    }
  }

  // Apply PWM Duty Cycle
  analogWrite(LM2596_PWM_PIN, pwmDutyCycle);

  // Check if charging has started or stopped
  if (pwmDutyCycle > 0 && !isCharging) {
    startChargingAlert(); // Trigger charging start alert
    isCharging = true;
  } else if (pwmDutyCycle == 0 && isCharging) {
    stopChargingAlert(); // Trigger charging stop alert
    isCharging = false;
  }

  // Display Real-Time Data
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Chg: ");
  lcd.print(chargeCurrent, 2); // Display current with 2 decimal places
  lcd.print("A");
  lcd.setCursor(0, 1);
  lcd.print("Bat: ");
  lcd.print(batteryVoltage, 2); // Display voltage with 2 decimal places
  lcd.print("V");
}

void stopCharging() {
  pwmDutyCycle = 0; // Stop PWM output
  analogWrite(LM2596_PWM_PIN, pwmDutyCycle);
  digitalWrite(LOAD_PIN, LOW); // Turn off load
}

void measureInternalResistance() {
  stopCharging();
  delay(100);

  // Measure Open-Circuit Voltage
  digitalWrite(LOAD_PIN, LOW);
  delay(100);
  openCircuitVoltage = ina219.getBusVoltage_V();

  // Measure Loaded Voltage
  digitalWrite(LOAD_PIN, HIGH);
  delay(100);
  loadedVoltage = ina219.getBusVoltage_V();
  loadCurrent = ina219.getCurrent_mA() / 1000.0;

  // Calculate Internal Resistance
  internalResistance = (openCircuitVoltage - loadedVoltage) / loadCurrent;

  digitalWrite(LOAD_PIN, LOW);

  // Display Results
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Int. Resistance:");
  lcd.setCursor(0, 1);
  lcd.print(internalResistance, 2); // Display resistance with 2 decimal places
  lcd.print(" Ohms");
  tone(BUZZER_PIN, 2000, 200); // Buzzer alert for internal resistance measurement
  delay(2000);
}

void updateDisplay() {
  lcd.clear();
  switch (menuState) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Chg: ");
      lcd.print(chargeCurrent, 2);
      lcd.print("A");
      lcd.setCursor(0, 1);
      lcd.print("Bat: ");
      lcd.print(batteryVoltage, 2);
      lcd.print("V");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Set Max V:");
      lcd.print(maxChargingVoltage, 1);
      lcd.setCursor(0, 1);
      lcd.print("Min V:");
      lcd.print(minChargingVoltage, 1);
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Measuring IR...");
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Save Settings?");
      lcd.setCursor(0, 1);
      lcd.print("Encoder: +/-");
      break;
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Main Menu");
      break;
  }
}

void saveSettingsToEEPROM() {
  EEPROM.put(0, maxChargingVoltage);
  EEPROM.put(4, minChargingVoltage);
}

void resetSettings() {
  maxChargingVoltage = 4.2;
  minChargingVoltage = 3.0;
  saveSettingsToEEPROM();
}

void startChargingAlert() {
  tone(BUZZER_PIN, 2000, 200); // High-pitched tone for charging start
  delay(300); // Short delay for clarity
  noTone(BUZZER_PIN); // Stop the tone
}

void stopChargingAlert() {
  tone(BUZZER_PIN, 500, 500); // Low-pitched tone for charging stop
  delay(600); // Longer delay for clarity
  noTone(BUZZER_PIN); // Stop the tone
}
