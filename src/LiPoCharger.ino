#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Adafruit_INA219.h>

// Include Modular Headers
#include "Calibration.h"
#include "Charging.h"
#include "Menu.h"
#include "Safety.h"
#include "Utils.h"

// Initialize Components
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_INA219 ina219;

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
  lcd.print("LiPo Charger");
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

  // Perform Automatic Calibration
  calibrateSystem();

  // Soft-Start Initialization
  softStart();
}

void loop() {
  wdt_reset(); // Reset the watchdog timer

  // Handle Menu Navigation
  handleMenu();

  // Update Display
  updateDisplay();

  // Charging Logic
  if (menuState == 0) {
    chargeBattery();
  }

  delay(100);
}
