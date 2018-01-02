#ifndef analog_read_task_hpp_INCLUDED
#define analog_read_task_hpp_INCLUDED

#include "Task.h"

class AnalogReadTask : public Task {
  public:
    AnalogReadTask(int pin, float repeatTime = 10);
    virtual void setup();
    virtual void run(unsigned long time) ;
    int getValue();

  private:
    int pin;
    int value;
};

#endif // analog_read_task_hpp_INCLUDED

