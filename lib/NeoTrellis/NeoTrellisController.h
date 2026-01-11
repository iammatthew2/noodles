/// @brief Manages 16-key NeoPixel button grid for visual feedback and input.

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

  // Optional external handler for key events
  void setKeyHandler(void (*handler)(uint8_t key, bool pressed));

  // Pixel helpers
  void setPixelColor(uint8_t keyIndex, uint32_t color);
  void clearPixels();
  void showPixels();
  uint32_t color(uint8_t r, uint8_t g, uint8_t b);

 private:
  // Hardware objects
  Adafruit_NeoTrellis trellis;

  // Pin assignments
  int _tonePin;

  // Optional handler for external consumers
  void (*_keyHandler)(uint8_t key, bool pressed) = nullptr;

  // Static instance pointer for callback
  static NeoTrellisController* instance;

  // Helper methods
  void runStartupAnimation();
  uint32_t wheel(byte wheelPos);
};

#endif
