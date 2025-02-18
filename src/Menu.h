#ifndef MENU_H
#define MENU_H

#include <Arduino.h>

int menuState = 0;
int encoderValue = 0;
int lastEncoderValue = 0;

void handleMenu() {
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
}

#endif
