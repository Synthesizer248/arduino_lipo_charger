#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include <Adafruit_INA219.h>

extern Adafruit_INA219 ina219;

float calibrationFactorVoltage = 1.0;
float calibrationFactorCurrent = 1.0;

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

void saveSettingsToEEPROM() {
  EEPROM.put(0, calibrationFactorVoltage);
  EEPROM.put(4, calibrationFactorCurrent);
}

#endif
