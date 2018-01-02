#include "attack_release_task.hpp"
#include <SPI.h>

#define ATTENUATOR_MAX 240 // it really jumps at the high end.. not smooth

AttackReleaseTask::AttackReleaseTask(int attackPotPin, int releasePotPin, int attenuatorCSPin) :
    Task("AttackReleaseTask", 10),
    attackPotPin(attackPotPin), releasePotPin(releasePotPin), attenuatorCSPin(attenuatorCSPin),
    startAttenuatorValue(0), state(AR_STATE_LIMBO),
    gateValue(LOW), attackValue(1), releaseValue(1) {
  stateStartTimeMillis = millis();
}

void AttackReleaseTask::setup() {
}

void AttackReleaseTask::setGate(int value) {
  gateValue = value;
}


// when used as a task, this only needs to be run
// every ms or two. The pot only has 8-bit granularity
void AttackReleaseTask::run(unsigned long time) {

  switch (state) {
    case AR_STATE_LIMBO:
      tickLimbo();
      break;
    case AR_STATE_ATTACK:
      tickAttack();
      break;
    case AR_STATE_SUSTAIN:
      tickSustain();
      break;
    case AR_STATE_RELEASE:
      tickRelease();
      break;
    default:
      state = AR_STATE_LIMBO;
      break;
  }
}

// waiting for input
void AttackReleaseTask::tickLimbo() {
  setAttenuator(0);
  // start attack when gate goes high
  if (gateValue == HIGH) {
    changeState(AR_STATE_ATTACK);
  }
}

// attack phase, volume is rising
void AttackReleaseTask::tickAttack() {
  int newAttenuatorValue = startAttenuatorValue + (float)ATTENUATOR_MAX/(float)attackValue*(millis() - stateStartTimeMillis);
  setAttenuator(min(ATTENUATOR_MAX, newAttenuatorValue));
  // enter release phase if gate goes low during attack
  if (gateValue == LOW) {
    startAttenuatorValue = newAttenuatorValue;
    changeState(AR_STATE_RELEASE);
  }
  // the attack phase finishes when the attenuator value maxes out
  else if (newAttenuatorValue >= ATTENUATOR_MAX) {
    startAttenuatorValue = ATTENUATOR_MAX;
    changeState(AR_STATE_SUSTAIN);
  }
}

// sustain phase, waiting for gate to go low
void AttackReleaseTask::tickSustain() {
  setAttenuator(ATTENUATOR_MAX);
  // enter release phase when gate goes low
  if (gateValue == LOW) {
    startAttenuatorValue = ATTENUATOR_MAX;
    changeState(AR_STATE_RELEASE);
  }
}

// release phase, volume is falling
void AttackReleaseTask::tickRelease() {
  int newAttenuatorValue = startAttenuatorValue-(float)ATTENUATOR_MAX/releaseValue*(millis() - stateStartTimeMillis);
  setAttenuator(max(0, newAttenuatorValue));
  // enter attack phase again if gate goes high during release
  if (gateValue == HIGH) {
    startAttenuatorValue = newAttenuatorValue;
    changeState(AR_STATE_ATTACK);
  }
  // the release phase finishes when attenuator value bottoms out
  else if (newAttenuatorValue <= 0) {
    startAttenuatorValue = 0;
    changeState(AR_STATE_LIMBO);
  }
}

void AttackReleaseTask::changeState(AR_STATE value) {
  state = value;
  stateStartTimeMillis = millis();
}

void AttackReleaseTask::setAttack(int value) {
  attackValue = value;
}

void AttackReleaseTask::setRelease(int value) {
  releaseValue = value;
}

void MCP41010Write(int csPin, byte value) {
  digitalWrite(csPin,LOW);
  SPI.transfer(B00010001); // This tells the chip to set the pot
  SPI.transfer(value);     // This tells it the pot position
  digitalWrite(csPin,HIGH);
}

void AttackReleaseTask::setAttenuator(byte value) {
  MCP41010Write(attenuatorCSPin, value);
}
