#ifndef SAFETY_H
#define SAFETY_H

#include <Arduino.h>

const float MAX_SAFE_CURRENT = 2.0;
const float MAX_BATTERY_VOLTAGE = 4.2;

void checkSafety() {
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
}

#endif
