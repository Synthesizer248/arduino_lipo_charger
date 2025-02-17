#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Pin Definitions
#define BUZZER_PIN 7
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_SW 4
#define LM2596_PWM_PIN 9

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
float maxChargingVoltage = 4.2; // Default
float minChargingVoltage = 3.0; // Default
float nominalChargingVoltage = 3.7; // Default
int ratedCapacitymAh = 1000; // Default
int typicalCapacitymAh = 1200; // Default
float chargeCurrent = 0.5; // Default in Amps
float shuntResistance = 0.1; // Shunt resistor value in Ohms
float batteryVoltage = 0;
float chargeTimeRequired = 0;
float healthIndex = 0;

// Calibration Variables
float calibrationFactorVoltage = 1.0; // Default calibration factor
float calibrationFactorCurrent = 1.0; // Default calibration factor

// Menu Variables
int menuState = 0; // 0: Main Menu, 1: Preset Settings, 2: Calibration, 3: Save/Reset
int encoderValue = 0;
int lastEncoderValue = 0;

void setup() {
  // Initialize Serial for Debugging
  Serial.begin(9600);

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  // Initialize PWM for LM2596
  pinMode(LM2596_PWM_PIN, OUTPUT);

  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("LiPo Charger");
  delay(2000);
  lcd.clear();

  // Load Settings from EEPROM
  loadSettingsFromEEPROM();

  // Set Analog Reference to Internal 1.1V
  analogReference(INTERNAL);
}

void loop() {
  // Read Encoder
  int clkState = digitalRead(ENCODER_CLK);
  int dtState = digitalRead(ENCODER_DT);

  if (clkState != lastEncoderValue) {
    if (dtState != clkState) {
      encoderValue++;
    } else {
      encoderValue--;
    }
    tone(BUZZER_PIN, 1000, 50); // Click feedback
  }
  lastEncoderValue = clkState;

  // Handle Menu Navigation
  handleMenu();

  // Update Display
  updateDisplay();

  // Charging Logic
  if (menuState == 0) { // Main Menu
    chargeBattery();
  }

  delay(100); // Debounce
}

void handleMenu() {
  // Check Encoder Switch Press
  if (digitalRead(ENCODER_SW) == LOW) {
    delay(200); // Debounce
    switch (menuState) {
      case 0: // Main Menu -> Enter Preset Settings
        menuState = 1;
        break;
      case 1: // Preset Settings -> Calibration
        menuState = 2;
        break;
      case 2: // Calibration -> Save/Reset
        menuState = 3;
        break;
      case 3: // Save/Reset -> Main Menu
        menuState = 0;
        break;
    }
  }

  // Handle Save/Reset in Save/Reset Menu
  if (menuState == 3) {
    if (encoderValue > 0) {
      saveSettingsToEEPROM();
      tone(BUZZER_PIN, 2000, 100); // Save feedback
      encoderValue = 0;
    } else if (encoderValue < 0) {
      resetSettings();
      tone(BUZZER_PIN, 500, 100); // Reset feedback
      encoderValue = 0;
    }
  }
}

void calibrateSystem() {
  // Step 1: Measure Known Reference Voltage
  float knownReferenceVoltage = 1.1; // Internal reference voltage
  int adcReading = analogRead(A0); // Read from A0 (connected to reference)
  float measuredVoltage = (adcReading / 1023.0) * 1.1; // Convert to voltage

  // Calculate Calibration Factor for Voltage
  calibrationFactorVoltage = knownReferenceVoltage / measuredVoltage;

  // Step 2: Measure Current via Shunt Resistor
  float knownCurrent = 0.5; // Example: Known current through shunt
  adcReading = analogRead(A1); // Read from A1 (shunt voltage)
  float measuredShuntVoltage = (adcReading / 1023.0) * 1.1;
  float measuredCurrent = measuredShuntVoltage / shuntResistance;

  // Calculate Calibration Factor for Current
  calibrationFactorCurrent = knownCurrent / measuredCurrent;

  // Feedback
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration Done!");
  tone(BUZZER_PIN, 2000, 200); // Calibration complete feedback
  delay(2000);
}

void updateDisplay() {
  lcd.clear();
  switch (menuState) {
    case 0: // Main Menu
      lcd.setCursor(0, 0);
      lcd.print("Chg: ");
      lcd.print(chargeCurrent);
      lcd.print("A");
      lcd.setCursor(0, 1);
      lcd.print("Bat: ");
      lcd.print(batteryVoltage);
      lcd.print("V");
      break;
    case 1: // Preset Settings
      lcd.setCursor(0, 0);
      lcd.print("Set Max V:");
      lcd.print(maxChargingVoltage);
      lcd.setCursor(0, 1);
      lcd.print("Min V:");
      lcd.print(minChargingVoltage);
      break;
    case 2: // Calibration Menu
      lcd.setCursor(0, 0);
      lcd.print("Calibrating...");
      calibrateSystem();
      menuState = 0; // Return to Main Menu after calibration
      break;
    case 3: // Save/Reset Menu
      lcd.setCursor(0, 0);
      lcd.print("Save Settings?");
      lcd.setCursor(0, 1);
      lcd.print("Encoder: +/-");
      break;
  }
}

void chargeBattery() {
  // Read Battery Voltage
  int adcReading = analogRead(A0);
  float rawVoltage = (adcReading / 1023.0) * 1.1;
  batteryVoltage = rawVoltage * calibrationFactorVoltage;

  // Read Current via Shunt Resistor
  adcReading = analogRead(A1);
  float rawShuntVoltage = (adcReading / 1023.0) * 1.1;
  float rawCurrent = rawShuntVoltage / shuntResistance;
  chargeCurrent = rawCurrent * calibrationFactorCurrent;

  // Control LM2596 for CV/CC
  // Add logic here to adjust PWM based on batteryVoltage and chargeCurrent
}

void loadSettingsFromEEPROM() {
  EEPROM.get(0, calibrationFactorVoltage);
  EEPROM.get(4, calibrationFactorCurrent);
}

void saveSettingsToEEPROM() {
  EEPROM.put(0, calibrationFactorVoltage);
  EEPROM.put(4, calibrationFactorCurrent);
}

void resetSettings() {
  calibrationFactorVoltage = 1.0;
  calibrationFactorCurrent = 1.0;
  saveSettingsToEEPROM();
}
