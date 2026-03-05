/// @brief Manages 16-key NeoPixel button grid for visual feedback and input.

#include "NeoTrellisController.h"

// Initialize static instance pointer
NeoTrellisController* NeoTrellisController::instance = nullptr;

NeoTrellisController::NeoTrellisController(int tonePin) : _tonePin(tonePin) {
  // Set static instance for callback
  instance = this;
}

void NeoTrellisController::setKeyHandler(void (*handler)(uint8_t key,
                                                         bool pressed)) {
  _keyHandler = handler;
}

bool NeoTrellisController::begin() {
  // Setup pins
  pinMode(_tonePin, OUTPUT);

  // Initialize NeoTrellis
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    return false;
  }

  // Cap LED brightness for better battery stability.
  trellis.pixels.setBrightness(64);

  Serial.println("NeoPixel Trellis started");

  // Activate all keys and set callbacks
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS; i++) {
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

void NeoTrellisController::update() { trellis.read(); }

TrellisCallback NeoTrellisController::keyCallback(keyEvent evt) {
  if (instance == nullptr) return 0;

  bool pressed = evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING;

  if (pressed) {
    tone(instance->_tonePin, 440 + (evt.bit.NUM * 50), 100);
    instance->trellis.pixels.setPixelColor(
        evt.bit.NUM,
        instance->wheel(
            map(evt.bit.NUM, 0, instance->trellis.pixels.numPixels(), 0, 255)));
  } else {
    instance->trellis.pixels.setPixelColor(evt.bit.NUM, 0);
  }

  instance->trellis.pixels.show();

  // Forward to external handler if present
  if (instance->_keyHandler) {
    instance->_keyHandler(evt.bit.NUM, pressed);
  }
  return 0;
}

void NeoTrellisController::runStartupAnimation() {
  // Light up sequence
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(
        i, wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(50);
  }
  // Clear sequence
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
}

uint32_t NeoTrellisController::wheel(byte wheelPos) {
  if (wheelPos < 85) {
    return trellis.pixels.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
  } else if (wheelPos < 170) {
    wheelPos -= 85;
    return trellis.pixels.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  } else {
    wheelPos -= 170;
    return trellis.pixels.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  return 0;
}

void NeoTrellisController::setPixelColor(uint8_t keyIndex, uint32_t color) {
  trellis.pixels.setPixelColor(keyIndex, color);
}

void NeoTrellisController::clearPixels() { trellis.pixels.clear(); }

void NeoTrellisController::showPixels() { trellis.pixels.show(); }

uint32_t NeoTrellisController::color(uint8_t r, uint8_t g, uint8_t b) {
  return trellis.pixels.Color(r, g, b);
}
