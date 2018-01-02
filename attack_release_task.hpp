#ifndef attack_release_task_h_INCLUDED
#define attack_release_task_h_INCLUDED

#include "Arduino.h"
#include "Task.h"

typedef enum {
  AR_STATE_LIMBO,   // between notes
  AR_STATE_ATTACK,  // during attack rise
  AR_STATE_SUSTAIN, // key still depressed/note still on
  AR_STATE_RELEASE  // during release fall
} AR_STATE;

class AttackReleaseTask : public Task {
public:
  AttackReleaseTask(int attackPotPin, int releasePotPin, int attenuatorCSPin);
  void setGate(int value);    //HIGH or LOW
  void setAttack(int value);  //0-1024ms
  void setRelease(int value); //0-1024ms
  virtual void setup();
  virtual void run(unsigned long time);

private:
  AR_STATE state;
  byte startAttenuatorValue;
  unsigned long stateStartTimeMillis;
  int attackPotPin, releasePotPin, attenuatorCSPin;
  int attackValue, releaseValue;
  int gateValue;

  void tickLimbo();
  void tickAttack();
  void tickSustain();
  void tickRelease();
  void changeState(AR_STATE value);
  void setAttenuator(byte value);
};

#endif // attack_release_task_h_INCLUDED

