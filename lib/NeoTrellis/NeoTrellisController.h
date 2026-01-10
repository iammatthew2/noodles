#ifndef NEOTRELLIS_CONTROLLER_H
#define NEOTRELLIS_CONTROLLER_H

#include <Arduino.h>
#include "Adafruit_NeoTrellis.h"
#include <RotaryEncoder.h>

class NeoTrellisController {
public:
  // Constructor
  NeoTrellisController(int encoder1PinA, int encoder1PinB, 
                       int encoder2PinA, int encoder2PinB,
                       int tonePin, int button1Pin, int button2Pin, int killSwitchPin);
  
  // Initialize hardware
  bool begin();
  
  // Update state (call in loop)
  void update();
  
  // Key event callback (must be static for C callback)
  static TrellisCallback keyCallback(keyEvent evt);

private:
  // Hardware objects
  Adafruit_NeoTrellis trellis;
  RotaryEncoder encoder1;
  RotaryEncoder encoder2;
  
  // Pin assignments
  int _tonePin;
  int _button1Pin;
  int _button2Pin;
  int _killSwitchPin;
  
  // State tracking
  int lastPos1;
  int lastPos2;
  int lastButton1State;
  int lastButton2State;
  
  // Static instance pointer for callback
  static NeoTrellisController* instance;
  
  // Helper methods
  void handleButton1();
  void handleButton2();
  void handleEncoder1();
  void handleEncoder2();
  void runStartupAnimation();
  uint32_t wheel(byte wheelPos);
};

#endif
