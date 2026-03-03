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

  // Selection logic uses encoder 1
  if (stateManager->isSelecting() && channel == 1) {
    int newIndex = (stateManager->getSelectedAppIndex() +
                    (direction > 0 ? 1 : -1) + stateManager->getAppCount()) %
                   stateManager->getAppCount();
    stateManager->setSelectedAppIndex(newIndex);
    const AppDefinition* app = stateManager->getCurrentApp();

    if (app && wifiMqttManager->isMqttConnected()) {
      char payload[96];
      snprintf(payload, sizeof(payload),
               "{\"state\":\"SELECTING\",\"app\":\"%s\"}", app->name);

      bool published = wifiMqttManager->publish(app->topic, payload);
      if (!published) {
        Serial.println("Failed to publish app-selection MQTT message");
      }
    }

    Serial.print("App selection -> ");
    Serial.println(stateManager->getApps()[newIndex].name);
    tone(tonePin, direction > 0 ? 700 : 500, 60);
    return;
  }

  // Log encoder event with app name if in SELECTING or CONTROL state
  if (!stateManager->isHome()) {
    const AppDefinition* app = stateManager->getCurrentApp();
    if (app) {
      Serial.print("Encoder ");
      Serial.print(channel);
      Serial.print(": ");
      Serial.print(app->name);
      Serial.print(" - ");
      Serial.println(direction > 0 ? "right" : "left");

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

  if (!stateManager->isInControlState()) {
    if (channel == 1) {
      tone(tonePin, direction > 0 ? 523 : 392, 50);  // C5 / G4
    } else if (channel == 2) {
      tone(tonePin, direction > 0 ? 659 : 330, 50);  // E5 / E4
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

  // Home state rainbow burst effect
  if (stateManager->isHome()) {
    Serial.print("Rainbow burst from button: ");
    Serial.println(key);

    // Create a fun rainbow emanating from the pressed button
    // Arrange 16 buttons in a 4x4 grid
    int row = key / 4;
    int col = key % 4;

    tone(tonePin, 800, 150);

    // Animate rainbow - light up buttons one at a time
    // 3 seconds / 16 buttons = ~187ms per button
    unsigned long startTime = millis();
    unsigned long totalDuration = 3000;  // 3 seconds
    unsigned long timePerButton = totalDuration / 16;

    for (uint8_t animStep = 0; animStep < 16; animStep++) {
      // Clear and redraw only buttons up to current animation step
      for (uint8_t i = 0; i < 16; i++) {
        int buttonRow = i / 4;
        int buttonCol = i % 4;

        // Manhattan distance from pressed button
        int distance = abs(buttonRow - row) + abs(buttonCol - col);

        // Only light up buttons with distance <= current animation step
        if (distance <= animStep) {
          // Create rainbow colors based on distance
          uint8_t hue = (distance * 60);
          uint8_t r, g, b;

          // Simple hue rotation with base color influence
          if (hue < 60) {
            r = 255;
            g = (hue * 255) / 60;
            b = 0;
          } else if (hue < 120) {
            r = ((120 - hue) * 255) / 60;
            g = 255;
            b = 0;
          } else if (hue < 180) {
            r = 0;
            g = 255;
            b = ((hue - 120) * 255) / 60;
          } else if (hue < 240) {
            r = 0;
            g = ((240 - hue) * 255) / 60;
            b = 255;
          } else {
            r = ((hue - 240) * 255) / 60;
            g = 0;
            b = 255;
          }

          // Blend with user color for subtle customization
          r = (r + homeButtonR) / 2;
          g = (g + homeButtonG) / 2;
          b = (b + homeButtonB) / 2;

          trellisController->setPixelColor(i,
                                           trellisController->color(r, g, b));
        } else {
          trellisController->setPixelColor(i,
                                           trellisController->color(0, 0, 0));
        }
      }

      trellisController->showPixels();

      // Fun ascending tones with a bit of variation
      int baseFreq = 400 + (animStep * 80);
      tone(tonePin, baseFreq, 80);

      // Wait for the time slice, but check if we're still on track
      unsigned long elapsed = millis() - startTime;
      unsigned long targetTime = (animStep + 1) * timePerButton;
      if (elapsed < targetTime) {
        delay(targetTime - elapsed);
      }
    }

    // Keep final rainbow visible briefly
    delay(200);

    // Fade back to the button being edited
    updateHomeDisplay();
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

  if (stateManager->isSelecting() && key == 15) {  // APP_SELECT_KEY = 15
    stateManager->enterControl();
  }
}
