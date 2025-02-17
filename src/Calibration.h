#ifndef CALIBRATION_H
#define CALIBRATION_H

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

#endif
