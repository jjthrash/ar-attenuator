#ifndef attack_release_task_h_INCLUDED
#define attack_release_task_h_INCLUDED

#include "Arduino.h"

typedef enum {
  AR_STATE_LIMBO,   // between notes
  AR_STATE_ATTACK,  // during attack rise
  AR_STATE_SUSTAIN, // key still depressed/note still on
  AR_STATE_RELEASE  // during release fall
} AR_STATE;

class AttackReleaseTask {
public:
  AttackReleaseTask(int attackPotPin, int releasePotPin, int attenuatorCSPin);
  void tick();
  void setGate(int value);

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
  void readKnobs();
};

#endif // attack_release_task_h_INCLUDED

