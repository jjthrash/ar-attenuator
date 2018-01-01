#include <SPI.h>
#include <MIDI.h>
#include <SoftwareSerial.h>
#include "pitches.h"
#include "attack_release_task.hpp"

#define MIDI_TX_PIN 6     // atmega pin 12 (PCINT22/OC0A/AIN0/PD6)
#define MIDI_RX_PIN 7     // atmega pin 13 (PCINT23/AIN1/PD7)
#define ATTENUATOR_CS 8   // atmega pin 14 (PCINT0/CLK0/ICP1/PB0) (I can choose this)
#define ATTENUATOR_SI 11  // atmega pin 17 (MOSI/OC2A/PCINT3/PB2) (seems invariant)
#define ATTENUATOR_SCK 13 // atmega pin 19 (SCK/PCINT5/PB5) (seems invariant)
#define AUDIO_OUT_PIN 10  // test purposes
#define GATE_PIN 5        // test purposes

#define ATTACK_PIN A0  // atmega pin 23
#define RELEASE_PIN A1 // atmega pin 24

AttackReleaseTask *attackReleaseTask = NULL;


SoftwareSerial SoftSerial(MIDI_RX_PIN,MIDI_TX_PIN);
MIDI_CREATE_INSTANCE(SoftwareSerial, SoftSerial, MIDI);

int midiGateState = LOW;

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {
Serial.println("note on");
// set tone
  tone(AUDIO_OUT_PIN, sNotePitches[inNote]);
// ar gate high
  midiGateState = HIGH;
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity) {
Serial.println("note off");
// ar gate low
  midiGateState = LOW;
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

  int newGate = (midiGateState == HIGH || gateButtonState == LOW) ? HIGH : LOW;
  attackReleaseTask->setGate(newGate);
  attackReleaseTask->tick();
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
