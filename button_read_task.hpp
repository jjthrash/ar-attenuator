#ifndef button_read_task_hpp_INCLUDED
#define button_read_task_hpp_INCLUDED

#include "Task.h"

class ButtonReadTask : public Task {
  public:
    ButtonReadTask(int pin);
    virtual void setup();
    virtual void run(unsigned long time) ;
    int isButtonPressed();

  private:
    int pin;
    int buttonState;
    int lastButtonState;
    unsigned long lastDebounceTime;
    unsigned long debounceDelay;
};

#endif // button_read_task_hpp_INCLUDED

