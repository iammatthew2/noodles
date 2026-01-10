#include "EncoderChannel.h"

EncoderChannel::EncoderChannel(int pinA, int pinB, int channelNum)
  : encoder(pinA, pinB, RotaryEncoder::LatchMode::TWO03),
    _channelNum(channelNum),
    _lastPosition(0),
    _callback(nullptr) {
}

void EncoderChannel::update() {
  encoder.tick();
  int newPos = encoder.getPosition();
  
  if (newPos != _lastPosition) {
    int direction = (newPos > _lastPosition) ? 1 : -1;
    
    if (_callback) {
      _callback(_channelNum, direction);
    }
    
    _lastPosition = newPos;
  }
}

int EncoderChannel::getPosition() {
  return _lastPosition;
}

void EncoderChannel::setCallback(void (*callback)(int channel, int direction)) {
  _callback = callback;
}
