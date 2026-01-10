#ifndef NEOTRELLIS_CONTROLLER_H
#define NEOTRELLIS_CONTROLLER_H

#include <Arduino.h>

#include "Adafruit_NeoTrellis.h"

class NeoTrellisController {
 public:
  // Constructor
  NeoTrellisController(int tonePin);

  // Initialize hardware
  bool begin();

  // Update state (call in loop)
  void update();

  // Key event callback (must be static for C callback)
  static TrellisCallback keyCallback(keyEvent evt);

 private:
  // Hardware objects
  Adafruit_NeoTrellis trellis;

  // Pin assignments
  int _tonePin;

  // Static instance pointer for callback
  static NeoTrellisController* instance;

  // Helper methods
  void runStartupAnimation();
  uint32_t wheel(byte wheelPos);
};

#endif
