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
      homeRowIndex(0),
      homeButtonR(255),
      homeButtonG(255),
      homeButtonB(255),
      homeColorRangeIndex(0) {}

void InputHandler::setNeoTrellis(NeoTrellisController* trellis) {
  trellisController = trellis;
}

void InputHandler::updateHomeDisplay() {
  if (!trellisController) return;

  // HOME encoder 1 selects one of four vertical columns.
  // Column order from right to left:
  // 0: keys 0,1,2,3
  // 1: keys 4,5,6,7
  // 2: keys 8,9,10,11
  // 3: keys 12,13,14,15
  static const uint8_t homeColumns[4][4] = {
      {0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11}, {12, 13, 14, 15}};

  // HOME encoder 2 selects one of four horizontal rows.
  // Row order from top to bottom:
  // 0: keys 0,4,8,12
  // 1: keys 1,5,9,13
  // 2: keys 2,6,10,14
  // 3: keys 3,7,11,15
  static const uint8_t homeRows[4][4] = {
      {0, 4, 8, 12}, {1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15}};

  trellisController->clearPixels();

  uint8_t columnIndex = homeButtonIndex % 4;
  uint8_t rowIndex = homeRowIndex % 4;
  uint32_t columnColor =
      trellisController->color(homeButtonR, homeButtonG, homeButtonB);
  for (uint8_t i = 0; i < 4; i++) {
    trellisController->setPixelColor(homeColumns[columnIndex][i], columnColor);
  }
  for (uint8_t i = 0; i < 4; i++) {
    trellisController->setPixelColor(homeRows[rowIndex][i], columnColor);
  }

  trellisController->showPixels();
}

void InputHandler::handleEncoderCallback(int channel, int direction) {
  // encoders edit local HOME UI only while in HOME.
  // Home state button control with encoder 1
  if (stateManager->isHome() && channel == 1) {
    // Business rule:
    // - Bar movement is clamped at column ends (no wraparound).
    // - After reaching an end, additional pushes against that limit cycle
    //   through color ranges that remain active while moving back inward.
    static const uint8_t homeColorRanges[][3] = {
        {255, 80, 80},    // red-ish
        {255, 180, 60},   // amber
        {240, 240, 80},   // yellow
        {80, 255, 120},   // green
        {80, 180, 255},   // blue
        {200, 120, 255},  // purple
        {255, 120, 200}   // pink
    };
    const uint8_t rangeCount =
        sizeof(homeColorRanges) / sizeof(homeColorRanges[0]);

    bool cycledColor = false;
    if (direction > 0) {
      if (homeButtonIndex < 3) {
        homeButtonIndex++;
      } else {
        homeColorRangeIndex = (homeColorRangeIndex + 1) % rangeCount;
        cycledColor = true;
      }
    } else {
      if (homeButtonIndex > 0) {
        homeButtonIndex--;
      } else {
        homeColorRangeIndex =
            (homeColorRangeIndex + rangeCount - 1) % rangeCount;
        cycledColor = true;
      }
    }

    if (cycledColor) {
      homeButtonR = homeColorRanges[homeColorRangeIndex][0];
      homeButtonG = homeColorRanges[homeColorRangeIndex][1];
      homeButtonB = homeColorRanges[homeColorRangeIndex][2];
    }

    int toneFreq = 700 + (homeButtonIndex * 120);
    tone(tonePin, toneFreq, 40);

    updateHomeDisplay();
    Serial.print("Home bar column: ");
    Serial.print(homeButtonIndex);
    if (cycledColor) {
      Serial.print(" color range -> ");
      Serial.println(homeColorRangeIndex);
    } else {
      Serial.println(direction > 0 ? " (right)" : " (left)");
    }
    return;
  }

  // Home state row control with encoder 2
  if (stateManager->isHome() && channel == 2) {
    static const uint8_t homeColorRanges[][3] = {
        {255, 80, 80},    // red-ish
        {255, 180, 60},   // amber
        {240, 240, 80},   // yellow
        {80, 255, 120},   // green
        {80, 180, 255},   // blue
        {200, 120, 255},  // purple
        {255, 120, 200}   // pink
    };
    const uint8_t rangeCount =
        sizeof(homeColorRanges) / sizeof(homeColorRanges[0]);

    bool cycledColor = false;
    if (direction > 0) {
      if (homeRowIndex < 3) {
        homeRowIndex++;
      } else {
        homeColorRangeIndex = (homeColorRangeIndex + 1) % rangeCount;
        cycledColor = true;
      }
    } else {
      if (homeRowIndex > 0) {
        homeRowIndex--;
      } else {
        homeColorRangeIndex =
            (homeColorRangeIndex + rangeCount - 1) % rangeCount;
        cycledColor = true;
      }
    }

    if (cycledColor) {
      homeButtonR = homeColorRanges[homeColorRangeIndex][0];
      homeButtonG = homeColorRanges[homeColorRangeIndex][1];
      homeButtonB = homeColorRanges[homeColorRangeIndex][2];
    }

    int toneFreq = 520 + (homeRowIndex * 110);
    tone(tonePin, toneFreq, 40);

    updateHomeDisplay();
    Serial.print("Home bar row: ");
    Serial.print(homeRowIndex);
    if (cycledColor) {
      Serial.print(" color range -> ");
      Serial.println(homeColorRangeIndex);
    } else {
      Serial.println(direction > 0 ? " (down)" : " (up)");
    }
    return;
  }

  // In selection mode, app choice is done via Trellis keys 0-3.
  if (stateManager->isSelecting()) {
    return;
  }

  // in CONTROL, encoder turns are forwarded as MQTT control
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

  // mute encoder tones during CONTROL; keep tones for HOME UX.
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
    homeRowIndex = 0;
    homeButtonR = 255;
    homeButtonG = 255;
    homeButtonB = 255;
    homeColorRangeIndex = 0;
    tone(tonePin, 500, 100);  // Reset tone
  }
}

void InputHandler::handleTrellisKey(uint8_t key, bool pressed) {
  if (!pressed) return;

  // app selection is explicit and limited to keys 0-3.
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
