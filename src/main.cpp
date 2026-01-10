#include <Arduino.h>
#include "NeoTrellisController.h"

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

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("\n\nNoodles Project");
  Serial.println("===============\n");
  
  // Initialize NeoTrellis controller
  trellisController = new NeoTrellisController(
    ENCODER1_PIN_A, ENCODER1_PIN_B,
    ENCODER2_PIN_A, ENCODER2_PIN_B,
    TONE_PIN, BUTTON1_PIN, BUTTON2_PIN, KILL_SWITCH_PIN
  );
  
  if (!trellisController->begin()) {
    Serial.println("Failed to initialize NeoTrellis controller!");
    while(1) delay(1);
  }
  
  Serial.println("System ready - new layout has been setup.");
}

void loop() {
  trellisController->update();
  delay(20);
}
