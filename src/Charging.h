#ifndef CHARGING_H
#define CHARGING_H

#include <Arduino.h>
#include <Adafruit_INA219.h>

extern Adafruit_INA219 ina219;

float batteryVoltage = 0.0;
float chargeCurrent = 0.0;
int pwmDutyCycle = 0;

void chargeBattery() {
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
    tone(BUZZER_PIN, 1000, 500);
    delay(2000);
    wdt_enable(WDTO_15MS);
    while (1);
  }

  // CC/CV Control Logic
  if (batteryVoltage < maxChargingVoltage) {
    if (chargeCurrent < targetChargeCurrent) {
      pwmDutyCycle = constrain(pwmDutyCycle + 1, 0, 255);
    } else if (chargeCurrent > targetChargeCurrent) {
      pwmDutyCycle = constrain(pwmDutyCycle - 1, 0, 255);
    }
  } else {
    if (batteryVoltage > maxChargingVoltage) {
      pwmDutyCycle = constrain(pwmDutyCycle - 1, 0, 255);
    } else if (batteryVoltage < maxChargingVoltage) {
      pwmDutyCycle = constrain(pwmDutyCycle + 1, 0, 255);
    }
  }

  analogWrite(LM2596_PWM_PIN, pwmDutyCycle);

  // Display Real-Time Data
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Chg: ");
  lcd.print(chargeCurrent, 2);
  lcd.print("A");
  lcd.setCursor(0, 1);
  lcd.print("Bat: ");
  lcd.print(batteryVoltage, 2);
  lcd.print("V");
}

void stopCharging() {
  pwmDutyCycle = 0;
  analogWrite(LM2596_PWM_PIN, pwmDutyCycle);
  digitalWrite(LOAD_PIN, LOW);
}

#endif
