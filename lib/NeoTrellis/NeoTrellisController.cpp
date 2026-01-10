#include "NeoTrellisController.h"

// Initialize static instance pointer
NeoTrellisController* NeoTrellisController::instance = nullptr;

NeoTrellisController::NeoTrellisController(int tonePin, int button1Pin, int button2Pin, int killSwitchPin)
  : _tonePin(tonePin),
    _button1Pin(button1Pin),
    _button2Pin(button2Pin),
    _killSwitchPin(killSwitchPin),
    lastButton1State(HIGH),
    lastButton2State(HIGH) {
  
  // Set static instance for callback
  instance = this;
}

bool NeoTrellisController::begin() {
  // Setup pins
  pinMode(_tonePin, OUTPUT);
  pinMode(_button1Pin, INPUT_PULLUP);
  pinMode(_button2Pin, INPUT_PULLUP);
  
  // Initialize NeoTrellis
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    return false;
  }
  
  Serial.println("NeoPixel Trellis started");
  
  // Activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, keyCallback);
  }
  
  // Run startup animation
  runStartupAnimation();
  
  Serial.println("NeoTrellis setup complete. Waiting for input...");
  tone(_tonePin, 523, 50);
  
  return true;
}

void NeoTrellisController::update() {
  trellis.read();
  handleButton1();
  handleButton2();
}

TrellisCallback NeoTrellisController::keyCallback(keyEvent evt) {
  if (instance == nullptr) return 0;
  
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    Serial.print("Pressed: ");
    Serial.println(evt.bit.NUM);
    tone(instance->_tonePin, 440 + (evt.bit.NUM * 50), 100);
    instance->trellis.pixels.setPixelColor(evt.bit.NUM, 
      instance->wheel(map(evt.bit.NUM, 0, instance->trellis.pixels.numPixels(), 0, 255)));
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    Serial.print("Released: ");
    Serial.println(evt.bit.NUM);
    instance->trellis.pixels.setPixelColor(evt.bit.NUM, 0);
  }
  
  instance->trellis.pixels.show();
  return 0;
}

void NeoTrellisController::handleButton1() {
  int button1State = digitalRead(_button1Pin);
  if (button1State == LOW && lastButton1State == HIGH) {
    Serial.println("Button 1 Pressed");
    tone(_tonePin, 440, 100); // A4
  }
  lastButton1State = button1State;
}

void NeoTrellisController::handleButton2() {
  int button2State = digitalRead(_button2Pin);
  if (button2State == LOW && lastButton2State == HIGH) {
    Serial.println("Button 2 Pressed");
    tone(_tonePin, 550, 100); // C#5
  }
  lastButton2State = button2State;
}

void NeoTrellisController::runStartupAnimation() {
  // Light up sequence
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(50);
  }
  // Clear sequence
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
}

uint32_t NeoTrellisController::wheel(byte wheelPos) {
  if(wheelPos < 85) {
    return trellis.pixels.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
  } else if(wheelPos < 170) {
    wheelPos -= 85;
    return trellis.pixels.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  } else {
    wheelPos -= 170;
    return trellis.pixels.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  return 0;
}
