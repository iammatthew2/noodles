/// @brief Handles rotary encoder input with positional tracking and tone
/// feedback.

#include "EncoderChannel.h"

EncoderChannel::EncoderChannel(int pinA, int pinB, int channelNum)
    : encoder(pinA, pinB, RotaryEncoder::LatchMode::TWO03),
      _channelNum(channelNum),
      _lastPosition(0),
      _logicalPosition(0),
      _pendingSteps(0),
      _callback(nullptr) {}

void EncoderChannel::update() {
  encoder.tick();
  int newPos = encoder.getPosition();
  int rawDelta = newPos - _lastPosition;

  if (rawDelta != 0) {
    _pendingSteps += rawDelta;

    while (abs(_pendingSteps) >= RAW_STEPS_PER_DETENT) {
      int direction = (_pendingSteps > 0) ? 1 : -1;
      _pendingSteps -= direction * RAW_STEPS_PER_DETENT;
      _logicalPosition += direction;

      // Call user callback if registered
      if (_callback) {
        _callback(_channelNum, direction);
      }
    }

    _lastPosition = newPos;
  }
}

int EncoderChannel::getPosition() { return _logicalPosition; }

void EncoderChannel::setCallback(void (*callback)(int channel, int direction)) {
  _callback = callback;
}
