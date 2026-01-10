#ifndef ENCODER_CHANNEL_H
#define ENCODER_CHANNEL_H

#include <Arduino.h>
#include <RotaryEncoder.h>

class EncoderChannel {
public:
  // Constructor
  EncoderChannel(int pinA, int pinB, int channelNum);
  
  // Update encoder state (call in loop)
  void update();
  
  // Get current position
  int getPosition();
  
  // Callback setter for position change events
  void setCallback(void (*callback)(int channel, int direction));

private:
  RotaryEncoder encoder;
  int _channelNum;
  int _lastPosition;
  void (*_callback)(int channel, int direction);
};

#endif
