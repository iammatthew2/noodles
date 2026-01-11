#include "InputHandler.h"

#include "NeoTrellisController.h"
#include "StateManager.h"
#include "WiFiMQTTManager.h"

InputHandler::InputHandler(StateManager* stateManager,
                           WiFiMQTTManager* wifiMqttManager, int tonePin)
    : stateManager(stateManager),
      wifiMqttManager(wifiMqttManager),
      trellisController(nullptr),
      tonePin(tonePin) {}

void InputHandler::setNeoTrellis(NeoTrellisController* trellis) {
  trellisController = trellis;
}

void InputHandler::handleEncoderCallback(int channel, int direction) {
  // Selection logic uses encoder 1
  if (stateManager->isSelecting() && channel == 1) {
    int newIndex = (stateManager->getSelectedAppIndex() +
                    (direction > 0 ? 1 : -1) + stateManager->getAppCount()) %
                   stateManager->getAppCount();
    stateManager->setSelectedAppIndex(newIndex);
    Serial.print("App selection -> ");
    Serial.println(stateManager->getApps()[newIndex].name);
    tone(tonePin, direction > 0 ? 700 : 500, 60);
    return;
  }

  // Default home behavior
  if (channel == 1) {
    if (direction > 0) {
      Serial.println("Encoder 1: CW");
      tone(tonePin, 523, 50);  // C5
    } else {
      Serial.println("Encoder 1: CCW");
      tone(tonePin, 392, 50);  // G4
    }
  } else if (channel == 2) {
    if (direction > 0) {
      Serial.println("Encoder 2: CW");
      tone(tonePin, 659, 50);  // E5
    } else {
      Serial.println("Encoder 2: CCW");
      tone(tonePin, 330, 50);  // E4
    }
  }
}

void InputHandler::handleButtonCallback(int buttonNum) {
  // Button 1: toggle between SELECTING and current state
  if (buttonNum == 1) {
    if (stateManager->isSelecting()) {
      stateManager->enterHome();
    } else if (stateManager->isHome()) {
      if (!wifiMqttManager->isMqttConnected()) {
        Serial.println("MQTT not connected - staying in home");
        tone(tonePin, 200, 200);  // Error tone
        return;
      }
      stateManager->enterSelecting();
    } else if (stateManager->isInControlState()) {
      stateManager->enterSelecting();
    }
    return;
  }

  // Button 2: reset to home state
  if (buttonNum == 2) {
    Serial.println("Button 2 pressed - resetting to home state");
    stateManager->enterHome();
    tone(tonePin, 500, 100);  // Reset tone
  }
}

void InputHandler::handleTrellisKey(uint8_t key, bool pressed) {
  if (!pressed) return;

  if (stateManager->isSelecting() && key == 15) {  // APP_SELECT_KEY = 15
    stateManager->enterControl();
  }
}
