#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <cstdint>

class StateManager;
class WiFiMQTTManager;

class InputHandler {
 public:
  InputHandler(StateManager* stateManager, WiFiMQTTManager* wifiMqttManager,
               int tonePin);

  void handleEncoderCallback(int channel, int direction);
  void handleButtonCallback(int buttonNum);
  void handleTrellisKey(uint8_t key, bool pressed);

  // Set NeoTrellis controller for display updates
  void setNeoTrellis(class NeoTrellisController* trellis);

 private:
  StateManager* stateManager;
  WiFiMQTTManager* wifiMqttManager;
  class NeoTrellisController* trellisController;
  int tonePin;
};

#endif
