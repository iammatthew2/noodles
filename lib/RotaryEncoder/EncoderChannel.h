/// @brief Handles rotary encoder input with positional tracking and tone
/// feedback.

#ifndef ENCODER_CHANNEL_H
#define ENCODER_CHANNEL_H

#include <Arduino.h>
#include <RotaryEncoder.h>

class EncoderChannel {
 public:
  // Constructor
  EncoderChannel(int pinA, int pinB, int channelNum, int tonePin = -1);

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
  int _tonePin;
  void (*_callback)(int channel, int direction);

  // Tone frequencies for feedback
  const int CW_TONE = 523;   // C5
  const int CCW_TONE = 392;  // G4
};

#endif
