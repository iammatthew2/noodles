#include <Arduino.h>
#include "NeoTrellisController.h"
#include "EncoderChannel.h"

// Pin definitions
const int ENCODER1_PIN_A = 3;
const int ENCODER1_PIN_B = 4;
const int ENCODER2_PIN_A = 5;
const int ENCODER2_PIN_B = 6;
const int TONE_PIN = 7;
const int BUTTON1_PIN = 8;
const int BUTTON2_PIN = 9;
const int KILL_SWITCH_PIN = 2;

// Component instances
NeoTrellisController* trellisController;
EncoderChannel* encoder1;
EncoderChannel* encoder2;

void encoderCallback(int channel, int direction) {
  if (channel == 1) {
    if (direction > 0) {
      Serial.println("Encoder 1: CW");
      tone(TONE_PIN, 523, 50); // C5
    } else {
      Serial.println("Encoder 1: CCW");
      tone(TONE_PIN, 392, 50); // G4
    }
  } else if (channel == 2) {
    if (direction > 0) {
      Serial.println("Encoder 2: CW");
      tone(TONE_PIN, 659, 50); // E5
    } else {
      Serial.println("Encoder 2: CCW");
      tone(TONE_PIN, 330, 50); // E4
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("\n\nNoodles Project");
  Serial.println("===============\n");
  
  // Initialize NeoTrellis controller
  trellisController = new NeoTrellisController(TONE_PIN, BUTTON1_PIN, BUTTON2_PIN, KILL_SWITCH_PIN);
  if (!trellisController->begin()) {
    Serial.println("Failed to initialize NeoTrellis controller!");
    while(1) delay(1);
  }
  
  // Initialize encoders
  encoder1 = new EncoderChannel(ENCODER1_PIN_A, ENCODER1_PIN_B, 1, TONE_PIN);
  encoder2 = new EncoderChannel(ENCODER2_PIN_A, ENCODER2_PIN_B, 2, TONE_PIN);
  
  encoder1->setCallback(encoderCallback);
  encoder2->setCallback(encoderCallback);
  
  Serial.println("System ready!");
}

void loop() {
  trellisController->update();
  encoder1->update();
  encoder2->update();
}
