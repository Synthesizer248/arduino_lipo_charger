#include <Wire.h> // Include the Wire library for I2C communication
#include <LiquidCrystal_I2C.h> // Include the LiquidCrystal_I2C library for the LCD display
#include <EEPROM.h> // Include the EEPROM library to save settings permanently

// Pin Definitions
#define BUZZER_PIN 7 // Pin connected to the buzzer for feedback sounds
#define LOAD_PIN 8 // Pin connected to the MOSFET to control the load resistor for internal resistance measurement
#define ENCODER_CLK 2 // Pin connected to the rotary encoder's CLK pin
#define ENCODER_DT 3 // Pin connected to the rotary encoder's DT pin
#define ENCODER_SW 4 // Pin connected to the rotary encoder's SW (push button) pin
#define LM2596_PWM_PIN 9 // Pin connected to the LM2596's feedback pin for PWM control

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD with I2C address 0x27 and set it to 16 columns and 2 rows

// Variables for Battery Charging
float maxChargingVoltage = 4.2; // Maximum charging voltage for the LiPo battery (default: 4.2V)
float minChargingVoltage = 3.0; // Minimum charging voltage for the LiPo battery (default: 3.0V)
float nominalChargingVoltage = 3.7; // Nominal charging voltage for the LiPo battery (default: 3.7V)
int ratedCapacitymAh = 1000; // Rated capacity of the battery in mAh (default: 1000mAh)
int typicalCapacitymAh = 1200; // Typical capacity of the battery in mAh (default: 1200mAh)
float chargeCurrent = 0.5; // Default charging current in Amps (default: 0.5A)
float shuntResistance = 0.1; // Resistance of the shunt resistor in Ohms (default: 0.1Ω)
float batteryVoltage = 0; // Variable to store the measured battery voltage
float chargeTimeRequired = 0; // Variable to store the estimated charging time
float healthIndex = 0; // Variable to store the calculated battery health index

// Calibration Variables
float calibrationFactorVoltage = 1.0; // Calibration factor for voltage measurements (default: 1.0)
float calibrationFactorCurrent = 1.0; // Calibration factor for current measurements (default: 1.0)

// Internal Resistance Measurement Variables
float openCircuitVoltage = 0; // Voltage across the battery when no load is applied
float loadedVoltage = 0; // Voltage across the battery when a load is applied
float internalResistance = 0; // Calculated internal resistance of the battery
float loadCurrent = 0; // Current through the load resistor during measurement
float loadResistance = 1.0; // Resistance of the load resistor in Ohms (default: 1Ω)

// Menu Variables
int menuState = 0; // Tracks the current menu state (0: Main Menu, 1: Preset Settings, 2: Calibration, 3: Save/Reset, 4: Internal Resistance)
int encoderValue = 0; // Tracks the encoder's value for navigation
int lastEncoderValue = 0; // Stores the previous encoder value to detect changes

void setup() {
  // Initialize Serial Communication for Debugging
  Serial.begin(9600); // Start serial communication at 9600 baud rate for debugging purposes

  // Initialize Pins
  pinMode(BUZZER_PIN, OUTPUT); // Set the buzzer pin as an output
  pinMode(LOAD_PIN, OUTPUT); // Set the load pin (MOSFET) as an output
  digitalWrite(LOAD_PIN, LOW); // Ensure the load is off initially by setting the MOSFET pin to LOW
  pinMode(ENCODER_CLK, INPUT_PULLUP); // Set the rotary encoder's CLK pin as an input with pull-up resistor
  pinMode(ENCODER_DT, INPUT_PULLUP); // Set the rotary encoder's DT pin as an input with pull-up resistor
  pinMode(ENCODER_SW, INPUT_PULLUP); // Set the rotary encoder's SW (button) pin as an input with pull-up resistor

  // Initialize PWM for LM2596 Buck Converter
  pinMode(LM2596_PWM_PIN, OUTPUT); // Set the LM2596's feedback pin as an output
  analogWrite(LM2596_PWM_PIN, 128); // Start with a 50% duty cycle (adjust this value as needed for your circuit)

  // Initialize LCD Display
  lcd.begin(); // Initialize the LCD display
  lcd.backlight(); // Turn on the backlight of the LCD
  lcd.clear(); // Clear the LCD screen
  lcd.print("LiPo Charger"); // Display a welcome message on the LCD
  delay(2000); // Wait for 2 seconds to allow the user to read the message
  lcd.clear(); // Clear the LCD screen after the welcome message

  // Load Saved Settings from EEPROM
  loadSettingsFromEEPROM(); // Load calibration factors and other saved settings from EEPROM

  // Set Analog Reference to Internal 1.1V
  analogReference(INTERNAL); // Use the Arduino's internal 1.1V reference for precise ADC readings
}

void loop() {
  // Read Rotary Encoder
  int clkState = digitalRead(ENCODER_CLK); // Read the state of the rotary encoder's CLK pin
  int dtState = digitalRead(ENCODER_DT); // Read the state of the rotary encoder's DT pin

  // Detect Encoder Rotation
  if (clkState != lastEncoderValue) { // Check if the encoder has been rotated
    if (dtState != clkState) { // Clockwise rotation increases the encoder value
      encoderValue++;
    } else { // Counterclockwise rotation decreases the encoder value
      encoderValue--;
    }
    tone(BUZZER_PIN, 1000, 50); // Provide audible feedback for each encoder step
  }
  lastEncoderValue = clkState; // Update the last encoder value for the next iteration

  // Handle Menu Navigation
  handleMenu(); // Call the function to handle menu navigation based on encoder input

  // Update Display
  updateDisplay(); // Call the function to update the LCD display based on the current menu state

  // Charging Logic
  if (menuState == 0) { // If the system is in the Main Menu (charging mode)
    chargeBattery(); // Call the function to handle the battery charging process
  }

  delay(100); // Add a small delay to debounce the encoder and prevent rapid changes
}

void handleMenu() {
  // Check if the Encoder Button is Pressed
  if (digitalRead(ENCODER_SW) == LOW) { // If the encoder button is pressed
    delay(200); // Debounce the button press by waiting 200ms
    switch (menuState) { // Navigate through the menu states
      case 0: // Main Menu -> Enter Preset Settings
        menuState = 1; // Switch to the Preset Settings menu
        break;
      case 1: // Preset Settings -> Calibration
        menuState = 2; // Switch to the Calibration menu
        break;
      case 2: // Calibration -> Save/Reset
        menuState = 3; // Switch to the Save/Reset menu
        break;
      case 3: // Save/Reset -> Internal Resistance
        menuState = 4; // Switch to the Internal Resistance menu
        break;
      case 4: // Internal Resistance -> Main Menu
        menuState = 0; // Return to the Main Menu
        break;
    }
  }

  // Handle Save/Reset in Save/Reset Menu
  if (menuState == 3) { // If the system is in the Save/Reset menu
    if (encoderValue > 0) { // If the encoder value is positive, save settings
      saveSettingsToEEPROM(); // Save the current calibration factors and settings to EEPROM
      tone(BUZZER_PIN, 2000, 100); // Provide audible feedback for saving
      encoderValue = 0; // Reset the encoder value
    } else if (encoderValue < 0) { // If the encoder value is negative, reset settings
      resetSettings(); // Reset all settings to their default values
      tone(BUZZER_PIN, 500, 100); // Provide audible feedback for resetting
      encoderValue = 0; // Reset the encoder value
    }
  }

  // Handle Internal Resistance Measurement
  if (menuState == 4) { // If the system is in the Internal Resistance menu
    measureInternalResistance(); // Measure the internal resistance of the battery
    menuState = 0; // Return to the Main Menu after measurement
  }
}

void calibrateSystem() {
  // Step 1: Measure Known Reference Voltage
  float knownReferenceVoltage = 1.1; // The internal reference voltage of the Arduino (1.1V)
  int adcReading = analogRead(A0); // Read the raw ADC value from the reference voltage pin (A0)
  float measuredVoltage = (adcReading / 1023.0) * 1.1; // Convert the raw ADC value to voltage using the internal reference

  // Calculate Calibration Factor for Voltage
  calibrationFactorVoltage = knownReferenceVoltage / measuredVoltage; // Adjust the calibration factor to account for any inaccuracies

  // Step 2: Measure Current via Shunt Resistor
  float knownCurrent = 0.5; // A known current value for calibration (e.g., 0.5A)
  adcReading = analogRead(A1); // Read the raw ADC value from the shunt resistor pin (A1)
  float measuredShuntVoltage = (adcReading / 1023.0) * 1.1; // Convert the raw ADC value to voltage using the internal reference
  float measuredCurrent = measuredShuntVoltage / shuntResistance; // Calculate the current using Ohm's Law (I = V/R)

  // Calculate Calibration Factor for Current
  calibrationFactorCurrent = knownCurrent / measuredCurrent; // Adjust the calibration factor to account for any inaccuracies

  // Feedback
  lcd.clear(); // Clear the LCD screen
  lcd.setCursor(0, 0); // Set the cursor to the first row
  lcd.print("Calibration Done!"); // Display a message indicating calibration is complete
  tone(BUZZER_PIN, 2000, 200); // Provide audible feedback for calibration completion
  delay(2000); // Wait for 2 seconds to allow the user to read the message
}

void measureInternalResistance() {
  // Disable Charging Circuit
  analogWrite(LM2596_PWM_PIN, 0); // Turn off the PWM signal to stop charging (disable the LM2596 buck converter)
  delay(100); // Allow the circuit to stabilize after disabling the charging process

  // Step 1: Measure Open-Circuit Voltage
  digitalWrite(LOAD_PIN, LOW); // Turn off the load by setting the MOSFET pin to LOW
  delay(100); // Allow the voltage to stabilize after removing the load
  int adcReading = analogRead(A0); // Read the raw ADC value from the battery voltage pin (A0)
  openCircuitVoltage = (adcReading / 1023.0) * 1.1 * calibrationFactorVoltage; // Convert the raw ADC value to voltage and apply the calibration factor

  // Step 2: Measure Loaded Voltage
  digitalWrite(LOAD_PIN, HIGH); // Turn on the load by setting the MOSFET pin to HIGH
  delay(100); // Allow the voltage to stabilize after applying the load
  adcReading = analogRead(A1); // Read the raw ADC value from the loaded voltage pin (A1)
  loadedVoltage = (adcReading / 1023.0) * 1.1 * calibrationFactorVoltage; // Convert the raw ADC value to voltage and apply the calibration factor

  // Step 3: Calculate Load Current
  loadCurrent = (openCircuitVoltage - loadedVoltage) / loadResistance; // Calculate the current through the load resistor using Ohm's Law (I = V/R)

  // Step 4: Calculate Internal Resistance
  internalResistance = (openCircuitVoltage - loadedVoltage) / loadCurrent; // Calculate the internal resistance of the battery using Ohm's Law (R = V/I)

  // Turn off the Load
  digitalWrite(LOAD_PIN, LOW); // Turn off the load by setting the MOSFET pin to LOW

  // Re-enable Charging Circuit
  analogWrite(LM2596_PWM_PIN, 128); // Restore the PWM signal to resume charging (50% duty cycle)

  // Display Results
  lcd.clear(); // Clear the LCD screen
  lcd.setCursor(0, 0); // Set the cursor to the first row
  lcd.print("Int. Resistance:"); // Display the label for internal resistance
  lcd.setCursor(0, 1); // Set the cursor to the second row
  lcd.print(internalResistance); // Display the calculated internal resistance
  lcd.print(" Ohms"); // Add the unit "Ohms" to the displayed value
  delay(2000); // Wait for 2 seconds to allow the user to read the result
}

void updateDisplay() {
  lcd.clear(); // Clear the LCD screen before updating the display
  switch (menuState) { // Update the display based on the current menu state
    case 0: // Main Menu
      lcd.setCursor(0, 0); // Set the cursor to the first row
      lcd.print("Chg: "); // Display the label for charging current
      lcd.print(chargeCurrent); // Display the current charging current
      lcd.print("A"); // Add the unit "A" (Amps) to the displayed value
      lcd.setCursor(0, 1); // Set the cursor to the second row
      lcd.print("Bat: "); // Display the label for battery voltage
      lcd.print(batteryVoltage); // Display the current battery voltage
      lcd.print("V"); // Add the unit "V" (Volts) to the displayed value
      break;
    case 1: // Preset Settings
      lcd.setCursor(0, 0); // Set the cursor to the first row
      lcd.print("Set Max V:"); // Display the label for maximum charging voltage
      lcd.print(maxChargingVoltage); // Display the current maximum charging voltage
      lcd.setCursor(0, 1); // Set the cursor to the second row
      lcd.print("Min V:"); // Display the label for minimum charging voltage
      lcd.print(minChargingVoltage); // Display the current minimum charging voltage
      break;
    case 2: // Calibration Menu
      lcd.setCursor(0, 0); // Set the cursor to the first row
      lcd.print("Calibrating..."); // Display a message indicating calibration is in progress
      calibrateSystem(); // Call the calibration function
      menuState = 0; // Return to the Main Menu after calibration
      break;
    case 3: // Save/Reset Menu
      lcd.setCursor(0, 0); // Set the cursor to the first row
      lcd.print("Save Settings?"); // Display a prompt for saving settings
      lcd.setCursor(0, 1); // Set the cursor to the second row
      lcd.print("Encoder: +/-"); // Display instructions for using the encoder to save or reset
      break;
    case 4: // Internal Resistance Menu
      lcd.setCursor(0, 0); // Set the cursor to the first row
      lcd.print("Measuring IR..."); // Display a message indicating internal resistance measurement is in progress
      break;
  }
}

void chargeBattery() {
  // Read Battery Voltage
  int adcReading = analogRead(A0); // Read the raw ADC value from the battery voltage pin (A0)
  float rawVoltage = (adcReading / 1023.0) * 1.1; // Convert the raw ADC value to voltage using the internal reference
  batteryVoltage = rawVoltage * calibrationFactorVoltage; // Apply the calibration factor to get the accurate battery voltage

  // Read Current via Shunt Resistor
  adcReading = analogRead(A1); // Read the raw ADC value from the shunt resistor pin (A1)
  float rawShuntVoltage = (adcReading / 1023.0) * 1.1; // Convert the raw ADC value to voltage using the internal reference
  float rawCurrent = rawShuntVoltage / shuntResistance; // Calculate the current using Ohm's Law (I = V/R)
  chargeCurrent = rawCurrent * calibrationFactorCurrent; // Apply the calibration factor to get the accurate charging current

  // Control LM2596 for CV/CC
  // Add logic here to adjust PWM based on batteryVoltage and chargeCurrent
}

void loadSettingsFromEEPROM() {
  EEPROM.get(0, calibrationFactorVoltage); // Load the voltage calibration factor from EEPROM address 0
  EEPROM.get(4, calibrationFactorCurrent); // Load the current calibration factor from EEPROM address 4
}

void saveSettingsToEEPROM() {
  EEPROM.put(0, calibrationFactorVoltage); // Save the voltage calibration factor to EEPROM address 0
  EEPROM.put(4, calibrationFactorCurrent); // Save the current calibration factor to EEPROM address 4
}

void resetSettings() {
  calibrationFactorVoltage = 1.0; // Reset the voltage calibration factor to its default value
  calibrationFactorCurrent = 1.0; // Reset the current calibration factor to its default value
  saveSettingsToEEPROM(); // Save the reset values to EEPROM
}
