#include <SPI.h>
#include <MIDI.h>
#include <SoftwareSerial.h>
#include "pitches.h"
#include "attack_release_task.hpp"
#include "button_read_task.hpp"
#include "analog_read_task.hpp"

#define MIDI_TX_PIN 6     // atmega pin 12 (PCINT22/OC0A/AIN0/PD6)
#define MIDI_RX_PIN 7     // atmega pin 13 (PCINT23/AIN1/PD7)
#define ATTENUATOR_CS 8   // atmega pin 14 (PCINT0/CLK0/ICP1/PB0) (I can choose this)
#define ATTENUATOR_SI 11  // atmega pin 17 (MOSI/OC2A/PCINT3/PB2) (seems invariant)
#define ATTENUATOR_SCK 13 // atmega pin 19 (SCK/PCINT5/PB5) (seems invariant)
#define AUDIO_OUT_PIN 10  // test purposes
#define GATE_PIN 5        // test purposes

#define ATTACK_PIN A0  // atmega pin 23
#define RELEASE_PIN A1 // atmega pin 24

AttackReleaseTask attackReleaseTask(ATTACK_PIN, RELEASE_PIN, ATTENUATOR_CS);
ButtonReadTask gateButtonReadTask(GATE_PIN);
AnalogReadTask attackKnobReadTask(ATTACK_PIN, 200);
//AnalogReadTask releaseKnobReadTask(RELEASE_PIN);

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
  tone(AUDIO_OUT_PIN, 110);

  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.begin();
//  MIDI.turnThruOn();

  TaskSystem.addTask(gateButtonReadTask);
  TaskSystem.addTask(attackKnobReadTask);
  //TaskSystem.addTask(releaseKnobReadTask);
  TaskSystem.addTask(attackReleaseTask);
}

void loop() {
  TaskSystem.execute();
  MIDI.read();
  attackReleaseTask.setGate((midiGateState == HIGH || gateButtonReadTask.isButtonPressed()) ? HIGH : LOW);
  attackReleaseTask.setAttack(attackKnobReadTask.getValue());
  attackReleaseTask.setRelease(500);
}

