#ifndef _FAIL_H
#define _FAIL_H

#include "config.h"

// when we fail, just loop infinitely
// and blink the LED quickly
void failLoop() {
  if(DEBUG) {
    Serial.println("Entered fail loop!");
  }

  // used to blink LED
  int LED_STATE = LOW;

  while(true) {
    digitalWrite(LED_BUILTIN, LED_STATE);
    if(LED_STATE == HIGH) {
      LED_STATE = LOW;
    } else {
      LED_STATE = HIGH;
    }
    delay(100);
  }
}

#endif // _FAIL_H