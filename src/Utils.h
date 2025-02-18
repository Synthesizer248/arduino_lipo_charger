#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

void softStart() {
  static bool softStartComplete = false;
  if (!softStartComplete) {
    for (int pwm = 0; pwm <= 255; pwm += 5) {
      analogWrite(LM2596_PWM_PIN, pwm);
      delay(50);
    }
    softStartComplete = true;
  }
}

void startChargingAlert() {
  tone(BUZZER_PIN, 2000, 200);
  delay(300);
  noTone(BUZZER_PIN);
}

void stopChargingAlert() {
  tone(BUZZER_PIN, 500, 500);
  delay(600);
  noTone(BUZZER_PIN);
}

#endif
