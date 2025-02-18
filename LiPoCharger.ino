#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Pin Definitions
#define BUZZER_PIN 7
#define LOAD_PIN 8
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_SW 4
#define LM2596_PWM_PIN 9

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
float maxChargingVoltage = 4.2;
float minChargingVoltage = 3.0;
float nominalChargingVoltage = 3.7;
int ratedCapacitymAh = 1000;
int typicalCapacitymAh = 1200;
float chargeCurrent = 0.5;
float shuntResistance = 0.1;
float batteryVoltage = 0;
float chargeTimeRequired = 0;
float healthIndex = 0;

float calibrationFactorVoltage = 1.0;
float calibrationFactorCurrent = 1.0;

float openCircuitVoltage = 0;
float loadedVoltage = 0;
float internalResistance = 0;
float loadCurrent = 0;
float loadResistance = 1.0;

int menuState = 0;
int encoderValue = 0;
int lastEncoderValue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);
  digitalWrite(LOAD_PIN, LOW);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(LM2596_PWM_PIN, OUTPUT);
  analogWrite(LM2596_PWM_PIN, 128);
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("LiPo Charger");
  delay(2000);
  lcd.clear();
  loadSettingsFromEEPROM();
  analogReference(INTERNAL);
}

void loop() {
  int clkState = digitalRead(ENCODER_CLK);
  int dtState = digitalRead(ENCODER_DT);

  if (clkState != lastEncoderValue) {
    if (dtState != clkState) {
      encoderValue++;
    } else {
      encoderValue--;
    }
    tone(BUZZER_PIN, 1000, 50);
  }
  lastEncoderValue = clkState;

  handleMenu();
  updateDisplay();

  if (menuState == 0) {
    chargeBattery();
  }

  delay(100);
}

void handleMenu() {
  if (digitalRead(ENCODER_SW) == LOW) {
    delay(200);
    switch (menuState) {
      case 0: menuState = 1; break;
      case 1: menuState = 2; break;
      case 2: menuState = 3; break;
      case 3: menuState = 4; break;
      case 4: menuState = 0; break;
    }
  }

  if (menuState == 3) {
    if (encoderValue > 0) {
      saveSettingsToEEPROM();
      tone(BUZZER_PIN, 2000, 100);
      encoderValue = 0;
    } else if (encoderValue < 0) {
      resetSettings();
      tone(BUZZER_PIN, 500, 100);
      encoderValue = 0;
    }
  }

  if (menuState == 2) {
    calibrateSystem();
    menuState = 0;
  }
}

void calibrateSystem() {
  calibrateVoltage();
  calibrateCurrent();
  saveSettingsToEEPROM();
}

void calibrateVoltage() {
  int adcReading = analogRead(A0);
  float measuredVoltage = (adcReading / 1023.0) * 1.1;
  float knownReferenceVoltage = 1.1;
  calibrationFactorVoltage = knownReferenceVoltage / measuredVoltage;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Voltage Calibrated!");
  tone(BUZZER_PIN, 2000, 200);
  delay(2000);
}

void calibrateCurrent() {
  analogWrite(LM2596_PWM_PIN, 0);
  delay(100);
  digitalWrite(LOAD_PIN, HIGH);
  delay(100);

  int adcReading = analogRead(A1);
  float measuredShuntVoltage = (adcReading / 1023.0) * 1.1;
  float measuredCurrent = measuredShuntVoltage / shuntResistance;

  float expectedCurrent = 1.0;
  calibrationFactorCurrent = expectedCurrent / measuredCurrent;

  digitalWrite(LOAD_PIN, LOW);
  analogWrite(LM2596_PWM_PIN, 128);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Calibrated!");
  tone(BUZZER_PIN, 2000, 200);
  delay(2000);
}

void measureInternalResistance() {
  analogWrite(LM2596_PWM_PIN, 0);
  delay(100);

  digitalWrite(LOAD_PIN, LOW);
  delay(100);
  int adcReading = analogRead(A0);
  openCircuitVoltage = (adcReading / 1023.0) * 1.1 * calibrationFactorVoltage;

  digitalWrite(LOAD_PIN, HIGH);
  delay(100);
  adcReading = analogRead(A1);
  loadedVoltage = (adcReading / 1023.0) * 1.1 * calibrationFactorVoltage;

  loadCurrent = (openCircuitVoltage - loadedVoltage) / loadResistance;
  internalResistance = (openCircuitVoltage - loadedVoltage) / loadCurrent;

  digitalWrite(LOAD_PIN, LOW);
  analogWrite(LM2596_PWM_PIN, 128);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Int. Resistance:");
  lcd.setCursor(0, 1);
  lcd.print(internalResistance);
  lcd.print(" Ohms");
  delay(2000);
}

void updateDisplay() {
  lcd.clear();
  switch (menuState) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Chg: ");
      lcd.print(chargeCurrent);
      lcd.print("A");
      lcd.setCursor(0, 1);
      lcd.print("Bat: ");
      lcd.print(batteryVoltage);
      lcd.print("V");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Set Max V:");
      lcd.print(maxChargingVoltage);
      lcd.setCursor(0, 1);
      lcd.print("Min V:");
      lcd.print(minChargingVoltage);
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Calibrating...");
      calibrateSystem();
      menuState = 0;
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Save Settings?");
      lcd.setCursor(0, 1);
      lcd.print("Encoder: +/-");
      break;
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Measuring IR...");
      break;
  }
}

void chargeBattery() {
  int adcReading = analogRead(A0);
  float rawVoltage = (adcReading / 1023.0) * 1.1;
  batteryVoltage = rawVoltage * calibrationFactorVoltage;

  adcReading = analogRead(A1);
  float rawShuntVoltage = (adcReading / 1023.0) * 1.1;
  float rawCurrent = rawShuntVoltage / shuntResistance;
  chargeCurrent = rawCurrent * calibrationFactorCurrent;
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
