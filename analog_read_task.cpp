#include "analog_read_task.hpp"
#include "Arduino.h"

AnalogReadTask::AnalogReadTask(int pin, float repeatTime) :
      Task("AnalogReadTask", repeatTime),
      pin(pin), value(value) {
}

void AnalogReadTask::setup() {
  value = analogRead(pin);
}

void AnalogReadTask::run(unsigned long time)  {
  value = analogRead(pin);
}

int AnalogReadTask::getValue() {
  return value;
}

