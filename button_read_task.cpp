#include "button_read_task.hpp"
#include "Arduino.h"


ButtonReadTask::ButtonReadTask(int pin) :
      Task("ButtonReadTask", 10),
      pin(pin),
      buttonState(HIGH), lastButtonState(HIGH),
      lastDebounceTime(0), debounceDelay(50) {
}

void ButtonReadTask::setup() {
  pinMode(pin, INPUT_PULLUP);
}

void ButtonReadTask::run(unsigned long time)  {
  int readButtonState = digitalRead(pin);

  if (readButtonState != lastButtonState) {
    lastDebounceTime = millis();
    lastButtonState = readButtonState;
  }

  if (millis() - lastDebounceTime > debounceDelay) {
    if (readButtonState != buttonState) {
      buttonState = readButtonState;
    }
  }
}

int ButtonReadTask::isButtonPressed() {
  return buttonState == LOW ? true : false;
}
