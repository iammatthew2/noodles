/// @brief Routes input events (encoder, button, trellis) to appropriate
/// handlers.

#include "InputHandler.h"

#include <ctype.h>

#include "NeoTrellisController.h"
#include "StateManager.h"
#include "WiFiMQTTManager.h"

InputHandler::InputHandler(StateManager* stateManager,
                           WiFiMQTTManager* wifiMqttManager, int tonePin)
    : stateManager(stateManager),
      wifiMqttManager(wifiMqttManager),
      trellisController(nullptr),
      tonePin(tonePin),
      homeButtonIndex(0),
      homeButtonIntensity(0),
      homeButtonR(255),
      homeButtonG(255),
      homeButtonB(255),
      homeColorMode(0) {}

void InputHandler::setNeoTrellis(NeoTrellisController* trellis) {
  trellisController = trellis;
}

void InputHandler::updateHomeDisplay() {
  if (!trellisController) return;

  trellisController->clearPixels();
  // Set the current button with current RGB color
  trellisController->setPixelColor(
      homeButtonIndex,
      trellisController->color(homeButtonR, homeButtonG, homeButtonB));
  trellisController->showPixels();
}

void InputHandler::handleEncoderCallback(int channel, int direction) {
  // Business rule: encoders edit local HOME UI only while in HOME.
  // Home state button control with encoder 1
  if (stateManager->isHome() && channel == 1) {
    // Move button selection
    if (direction > 0) {
      homeButtonIndex = (homeButtonIndex + 1) % 16;
      homeButtonIntensity =
          (homeButtonIntensity + 15 > 255) ? 255 : homeButtonIntensity + 15;
    } else {
      homeButtonIndex = (homeButtonIndex - 1 + 16) % 16;
      homeButtonIntensity =
          (homeButtonIntensity < 15) ? 0 : homeButtonIntensity - 15;
    }

    // Map intensity to tone (0-255 -> 200-2000 Hz)
    int toneFreq = 200 + (homeButtonIntensity * 7);
    tone(tonePin, toneFreq, 40);

    updateHomeDisplay();
    Serial.print("Home button control: Button ");
    Serial.print(homeButtonIndex);
    Serial.print(" Intensity: ");
    Serial.println(homeButtonIntensity);
    return;
  }

  // Home state color control with encoder 2
  if (stateManager->isHome() && channel == 2) {
    uint8_t step = 15;

    // Adjust current color channel based on direction
    if (homeColorMode == 0) {  // Red channel
      homeButtonR =
          (direction > 0)
              ? ((homeButtonR + step > 255) ? 255 : homeButtonR + step)
              : ((homeButtonR < step) ? 0 : homeButtonR - step);
    } else if (homeColorMode == 1) {  // Green channel
      homeButtonG =
          (direction > 0)
              ? ((homeButtonG + step > 255) ? 255 : homeButtonG + step)
              : ((homeButtonG < step) ? 0 : homeButtonG - step);
    } else {  // Blue channel
      homeButtonB =
          (direction > 0)
              ? ((homeButtonB + step > 255) ? 255 : homeButtonB + step)
              : ((homeButtonB < step) ? 0 : homeButtonB - step);
    }

    // Map current value to tone
    uint8_t currentValue = (homeColorMode == 0)   ? homeButtonR
                           : (homeColorMode == 1) ? homeButtonG
                                                  : homeButtonB;
    int toneFreq = 200 + (currentValue * 7);
    tone(tonePin, toneFreq, 40);

    updateHomeDisplay();
    const char* colorNames[] = {"Red", "Green", "Blue"};
    Serial.print("Home ");
    Serial.print(colorNames[homeColorMode]);
    Serial.print(": ");
    Serial.println(currentValue);
    return;
  }

  // In selection mode, app choice is done via Trellis keys 0-3.
  if (stateManager->isSelecting()) {
    return;
  }

  // Business rule: in CONTROL, encoder turns are forwarded as MQTT control
  // events to the currently selected app topic.
  if (!stateManager->isHome()) {
    const AppDefinition* app = stateManager->getCurrentApp();
    if (app) {
      if (wifiMqttManager->isMqttConnected()) {
        char appNameLower[24];
        size_t i = 0;
        for (; app->name[i] != '\0' && i < sizeof(appNameLower) - 1; ++i) {
          appNameLower[i] = tolower((unsigned char)app->name[i]);
        }
        appNameLower[i] = '\0';

        char payload[48];
        snprintf(payload, sizeof(payload), "enc%d-%s-%s", channel, appNameLower,
                 direction > 0 ? "right" : "left");

        bool published = wifiMqttManager->publish(app->topic, payload);
        if (!published) {
          Serial.println("Failed to publish encoder MQTT message");
        }
      }
    }
  } else if (channel == 1) {
    Serial.print("Encoder 1: ");
    Serial.println(direction > 0 ? "right" : "left");
  } else if (channel == 2) {
    Serial.print("Encoder 2: ");
    Serial.println(direction > 0 ? "right" : "left");
  }

  // Business rule: mute encoder tones during CONTROL; keep tones for HOME UX.
  if (!stateManager->isInControlState()) {
    if (channel == 1) {
      tone(tonePin, direction > 0 ? 523 : 392, 50);  // C5 / G4
    } else if (channel == 2) {
      tone(tonePin, direction > 0 ? 659 : 330, 50);  // E5 / E4
    }
  }
}

void InputHandler::handleButtonCallback(int buttonNum) {
  // Business rule:
  // - Button 1 toggles between HOME/SELECTING and can exit CONTROL to
  // SELECTING.
  // - Button 2 is a hard reset back to HOME defaults.
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
    homeButtonIndex = 0;
    homeButtonIntensity = 0;
    homeButtonR = 255;
    homeButtonG = 255;
    homeButtonB = 255;
    homeColorMode = 0;
    tone(tonePin, 500, 100);  // Reset tone
  }
}

void InputHandler::handleTrellisKey(uint8_t key, bool pressed) {
  if (!pressed) return;

  // Business rule: app selection is explicit and limited to keys 0-3.
  // Choosing a valid key enters CONTROL immediately.
  if (stateManager->isSelecting()) {
    int selectedIndex = -1;
    if (key == 0) {
      selectedIndex = 1;  // Skippy
    } else if (key == 1) {
      selectedIndex = 3;  // Pickles
    } else if (key == 2) {
      selectedIndex = 4;  // Puddles
    } else if (key == 3) {
      selectedIndex = 0;  // Yodel
    }

    if (selectedIndex >= 0 && selectedIndex < stateManager->getAppCount()) {
      stateManager->setSelectedAppIndex(selectedIndex);
      Serial.print("Selecting confirmed: ");
      Serial.println(stateManager->getApps()[selectedIndex].name);
      stateManager->enterControl();
    }
    return;
  }

  // Home state: no rainbow burst on button press
  if (stateManager->isHome()) {
    return;
  }

  if (!stateManager->isHome()) {
    const AppDefinition* app = stateManager->getCurrentApp();
    if (app) {
      Serial.print("NeoTrellis Key ");
      Serial.print(key);
      Serial.print(": ");
      Serial.println(app->name);

      // Publish MQTT message for trellis key press
      if (wifiMqttManager->isMqttConnected()) {
        char payload[32];
        snprintf(payload, sizeof(payload), "{\"key\":%d,\"pressed\":true}",
                 key);
        bool published = wifiMqttManager->publish(app->topic, payload);
        if (published) {
          Serial.print("Published to ");
          Serial.print(app->topic);
          Serial.print(": ");
          Serial.println(payload);
        } else {
          Serial.println("Failed to publish MQTT message");
        }
      }
    }
  }
}
