#include <SPI.h>
#include <MIDI.h>
#include <SoftwareSerial.h>
#include "pitches.h"

#define MIDI_TX_PIN 6     // atmega pin 12 (PCINT22/OC0A/AIN0/PD6)
#define MIDI_RX_PIN 7     // atmega pin 13 (PCINT23/AIN1/PD7)
#define ATTENUATOR_CS 8   // atmega pin 14 (PCINT0/CLK0/ICP1/PB0) (I can choose this)
#define ATTENUATOR_SI 11  // atmega pin 17 (MOSI/OC2A/PCINT3/PB2) (seems invariant)
#define ATTENUATOR_SCK 13 // atmega pin 19 (SCK/PCINT5/PB5) (seems invariant)
#define AUDIO_OUT_PIN 10  // test purposes
#define GATE_PIN 5        // test purposes

#define ATTACK_PIN A0  // atmega pin 23
#define RELEASE_PIN A1 // atmega pin 24

#define ATTENUATOR_MAX 240 // it really jumps at the high end.. not smooth

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

AttackReleaseTask *attackReleaseTask = NULL;


SoftwareSerial SoftSerial(MIDI_RX_PIN,MIDI_TX_PIN);
MIDI_CREATE_INSTANCE(SoftwareSerial, SoftSerial, MIDI);


void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {
Serial.println("note on");
// set tone
  tone(AUDIO_OUT_PIN, sNotePitches[inNote]);
// ar gate high
  attackReleaseTask->setGate(HIGH);
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity) {
Serial.println("note off");
// ar gate low
  attackReleaseTask->setGate(LOW);
}

void setup() {
  pinMode(ATTENUATOR_CS, OUTPUT);
  pinMode(GATE_PIN, INPUT_PULLUP);
  digitalWrite(ATTENUATOR_CS, HIGH);
  SPI.begin();
  Serial.begin(19200);
  attackReleaseTask = new AttackReleaseTask(ATTACK_PIN, RELEASE_PIN, ATTENUATOR_CS);
  tone(AUDIO_OUT_PIN, 110);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin();
//  MIDI.turnThruOn();
}

int gateButtonState = HIGH;
int lastGateButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void loop() {
  attackReleaseTask->tick();
  MIDI.read();

  int readButtonState = digitalRead(GATE_PIN);
  if (readButtonState != lastGateButtonState) {
    lastDebounceTime = millis();
    lastGateButtonState = readButtonState;
  }

  if (millis() - lastDebounceTime > debounceDelay) {
    if (readButtonState != gateButtonState) {
      Serial.println("button state changed");
      gateButtonState = readButtonState;
    }
  }
}

AttackReleaseTask::AttackReleaseTask(int attackPotPin, int releasePotPin, int attenuatorCSPin) :
    attackPotPin(attackPotPin), releasePotPin(releasePotPin), attenuatorCSPin(attenuatorCSPin),
    startAttenuatorValue(0), state(AR_STATE_LIMBO), gateValue(LOW) {
  stateStartTimeMillis = millis();
  readKnobs();
}

void AttackReleaseTask::setGate(int value) {
  gateValue = value;
}

void AttackReleaseTask::readKnobs() {
  attackValue = analogRead(attackPotPin);
  releaseValue = 500; //analogRead(releasePotPin);
}

// when used as a task, this only needs to be run
// every ms or two. The pot only has 8-bit granularity
void AttackReleaseTask::tick() {
  if (millis() % 10 == 0) { // every 10ms
    readKnobs();
  }

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

byte attenuatorLevel() {
  return (byte)(245.0*(sin((float)millis()/50)+1)/2.0);
}

void AttackReleaseTask::setAttenuator(byte value) {
  MCP41010Write(attenuatorCSPin, value);
}

void MCP41010Write(int csPin, byte value) {
  digitalWrite(csPin,LOW);
  SPI.transfer(B00010001); // This tells the chip to set the pot
  SPI.transfer(value);     // This tells it the pot position
  digitalWrite(csPin,HIGH);
}
