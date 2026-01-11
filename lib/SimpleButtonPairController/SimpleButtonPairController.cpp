/// @brief Manages two digital button inputs with tone feedback.

#include "SimpleButtonPairController.h"

SimpleButtonPairController::SimpleButtonPairController(int button1Pin,
                                                       int button2Pin,
                                                       int tonePin)
    : _button1Pin(button1Pin),
      _button2Pin(button2Pin),
      _tonePin(tonePin),
      lastButton1State(HIGH),
      lastButton2State(HIGH),
      _callback(nullptr) {
  Serial.println("SimpleButtonPairController instantiated");
}

void SimpleButtonPairController::begin() {
  pinMode(_button1Pin, INPUT_PULLUP);
  pinMode(_button2Pin, INPUT_PULLUP);
}

void SimpleButtonPairController::update() {
  // Handle button 1
  int button1State = digitalRead(_button1Pin);
  if (button1State == LOW && lastButton1State == HIGH) {
    Serial.println("Button 1 Pressed");

    // Play tone if configured
    if (_tonePin >= 0) {
      tone(_tonePin, BUTTON1_TONE, 100);
    }

    // Call user callback if registered
    if (_callback) {
      _callback(1);
    }
  }
  lastButton1State = button1State;

  // Handle button 2
  int button2State = digitalRead(_button2Pin);
  if (button2State == LOW && lastButton2State == HIGH) {
    Serial.println("Button 2 Pressed");

    // Play tone if configured
    if (_tonePin >= 0) {
      tone(_tonePin, BUTTON2_TONE, 100);
    }

    // Call user callback if registered
    if (_callback) {
      _callback(2);
    }
  }
  lastButton2State = button2State;
}

void SimpleButtonPairController::setCallback(void (*callback)(int buttonNum)) {
  _callback = callback;
}
