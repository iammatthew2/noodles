/// @brief Handles rotary encoder input with positional tracking and tone
/// feedback.

#include "EncoderChannel.h"

EncoderChannel::EncoderChannel(int pinA, int pinB, int channelNum, int tonePin)
    : encoder(pinA, pinB, RotaryEncoder::LatchMode::TWO03),
      _channelNum(channelNum),
      _lastPosition(0),
      _tonePin(tonePin),
      _callback(nullptr) {}

void EncoderChannel::update() {
  encoder.tick();
  int newPos = encoder.getPosition();

  if (newPos != _lastPosition) {
    int direction = (newPos > _lastPosition) ? 1 : -1;

    // Immediate tone feedback for responsiveness
    if (_tonePin >= 0) {
      tone(_tonePin, (direction > 0) ? CW_TONE : CCW_TONE, 50);
    }

    // Call user callback if registered
    if (_callback) {
      _callback(_channelNum, direction);
    }

    _lastPosition = newPos;
  }
}

int EncoderChannel::getPosition() { return _lastPosition; }

void EncoderChannel::setCallback(void (*callback)(int channel, int direction)) {
  _callback = callback;
}
