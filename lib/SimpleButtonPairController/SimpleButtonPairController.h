#ifndef SIMPLE_BUTTON_PAIR_CONTROLLER_H
#define SIMPLE_BUTTON_PAIR_CONTROLLER_H

#include <Arduino.h>

class SimpleButtonPairController {
 public:
  // Constructor
  SimpleButtonPairController(int button1Pin, int button2Pin, int tonePin = -1);

  // Initialize hardware
  void begin();

  // Update state (call in loop)
  void update();

  // Callback setter for button press events
  void setCallback(void (*callback)(int buttonNum));

 private:
  int _button1Pin;
  int _button2Pin;
  int _tonePin;

  int lastButton1State;
  int lastButton2State;

  void (*_callback)(int buttonNum);

  // Tone frequencies for feedback
  const int BUTTON1_TONE = 440;  // A4
  const int BUTTON2_TONE = 550;  // C#5
};

#endif
