#include <SPI.h>

#define ATTENUATOR_CS 8   // atmega pin 14 (PCINT0/CLK0/ICP1/PB0) (I can choose this)
#define ATTENUATOR_SI 11  // atmega pin 17 (MOSI/OC2A/PCINT3/PB2) (seems invariant)
#define ATTENUATOR_SCK 13 // atmega pin 19 (SCK/PCINT5/PB5) (seems invariant)
#define AUDIO_OUT_PIN 10  // test purposes
#define GATE_PIN 5        // test purposes

#define ATTACK_PIN A0 // atmega pin 23

typedef enum {
  AR_STATE_LIMBO,   // between notes
  AR_STATE_ATTACK,  // during attack rise
  AR_STATE_SUSTAIN, // key still depressed/note still on
  AR_STATE_RELEASE  // during release fall
} AR_STATE;

unsigned long stateStartTimeMillis = 0;
byte stateStartAttenuatorValue     = 0;
AR_STATE arState                   = AR_STATE_LIMBO;

void setup() {
  pinMode(ATTENUATOR_CS, OUTPUT);
  pinMode(GATE_PIN, INPUT);
  digitalWrite(ATTENUATOR_CS, HIGH);
  SPI.begin();
  Serial.begin(19200);
  tone(AUDIO_OUT_PIN, 110);
}

void loop() {
  arTick();
}

// when used as a task, this only needs to be run
// every ms or two. The pot only has 8-bit granularity
void arTick() {
  if (millis() % 1000 == 0) {
    Serial.println("state");
    Serial.println(arState);
  }
  switch (arState) {
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
      arState = AR_STATE_LIMBO;
      break;
  }
}

// waiting for input
void tickLimbo() {
  setAttenuator(0);
  // start attack when gate goes high
  if (gate() == HIGH) {
    changeState(AR_STATE_ATTACK);
  }
}

// attack phase, volume is rising
void tickAttack() {
  int attack = attackValue();
  int newAttenuatorValue = stateStartAttenuatorValue + 240.0/(float)attack*(millis() - stateStartTimeMillis);
  setAttenuator(min(240, newAttenuatorValue));
  // enter release phase if gate goes low during attack
  if (gate() == LOW) {
    stateStartAttenuatorValue = newAttenuatorValue;
    changeState(AR_STATE_RELEASE);
  }
  // the attack phase finishes when the attenuator value maxes out
  else if (newAttenuatorValue >= 240) {
    stateStartAttenuatorValue = 240;
    changeState(AR_STATE_SUSTAIN);
  }
}

// sustain phase, waiting for gate to go low
void tickSustain() {
  setAttenuator(240);
  // enter release phase when gate goes low
  if (gate() == LOW) {
    stateStartAttenuatorValue = 240;
    changeState(AR_STATE_RELEASE);
  }
}

// release phase, volume is falling
void tickRelease() {
  int release = releaseValue();
  int newAttenuatorValue = stateStartAttenuatorValue-240.0/release*(millis() - stateStartTimeMillis);
  setAttenuator(max(0, newAttenuatorValue));
  // enter attack phase again if gate goes high during release
  if (gate() == HIGH) {
    stateStartAttenuatorValue = newAttenuatorValue;
    changeState(AR_STATE_ATTACK);
  }
  // the release phase finishes when attenuator value bottoms out
  else if (newAttenuatorValue <= 0) {
    stateStartAttenuatorValue = 0;
    changeState(AR_STATE_LIMBO);
  }
}

void changeState(AR_STATE value) {
  arState = value;
  stateStartTimeMillis = millis();
}

int attackValue() {
  return analogRead(ATTACK_PIN); //ms
}

int releaseValue() {
  return 500; //ms
}

int gate() {
  return digitalRead(GATE_PIN);
}

byte attenuatorLevel() {
  return (byte)(245.0*(sin((float)millis()/50)+1)/2.0);
}

void setAttenuator(byte value) {
  MCP41010Write(value);
}

void MCP41010Write(byte value) {
  digitalWrite(ATTENUATOR_CS,LOW);
  SPI.transfer(B00010001); // This tells the chip to set the pot
  SPI.transfer(value);     // This tells it the pot position
  digitalWrite(ATTENUATOR_CS,HIGH);
}
