#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiNINA.h>

#include "EncoderChannel.h"
#include "NeoTrellisController.h"
#include "SimpleButtonPairController.h"
#include "StateManager.h"
#include "secrets.h"

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// MQTT configuration
const char* mqttBroker = "192.168.0.132";  // Global broker
const uint16_t mqttPort = 1889;  // Adjust if your broker uses a different port

// App definitions
AppDefinition apps[] = {
    {"Yodel", "apps/yodel/control", 255, 80, 80},
    {"Skippy", "apps/skippy/control", 80, 255, 120},
    {"Jibbers", "apps/jibbers/control", 80, 180, 255},
    {"Pickles", "apps/pickles/control", 255, 200, 80},
};

StateManager* stateManager;
// Forward declarations
void handleTrellisKey(uint8_t key, bool pressed);
bool ensureMqttConnected();
void pollConnectivity();
unsigned long lastConnectivityCheckMs = 0;
const unsigned long CONNECTIVITY_CHECK_INTERVAL_MS = 5000;

// Pin definitions
const int ENCODER1_PIN_A = 3;
const int ENCODER1_PIN_B = 4;
const int ENCODER2_PIN_A = 5;
const int ENCODER2_PIN_B = 6;
const int TONE_PIN = 7;
const int BUTTON1_PIN = 9;
const int BUTTON2_PIN = 8;
const int KILL_SWITCH_PIN = 2;

// Component instances
NeoTrellisController* trellisController;
EncoderChannel* encoder1;
EncoderChannel* encoder2;
SimpleButtonPairController* simpleButtonPairController;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void encoderCallback(int channel, int direction) {
  // Selection logic uses encoder 1
  if (stateManager->isSelecting() && channel == 1) {
    int newIndex = (stateManager->getSelectedAppIndex() +
                    (direction > 0 ? 1 : -1) + stateManager->getAppCount()) %
                   stateManager->getAppCount();
    stateManager->setSelectedAppIndex(newIndex);
    Serial.print("App selection -> ");
    Serial.println(stateManager->getApps()[newIndex].name);
    tone(TONE_PIN, direction > 0 ? 700 : 500, 60);
    return;
  }

  // Default home behavior
  if (channel == 1) {
    if (direction > 0) {
      Serial.println("Encoder 1: CW");
      tone(TONE_PIN, 523, 50);  // C5
    } else {
      Serial.println("Encoder 1: CCW");
      tone(TONE_PIN, 392, 50);  // G4
    }
  } else if (channel == 2) {
    if (direction > 0) {
      Serial.println("Encoder 2: CW");
      tone(TONE_PIN, 659, 50);  // E5
    } else {
      Serial.println("Encoder 2: CCW");
      tone(TONE_PIN, 330, 50);  // E4
    }
  }
}

void buttonCallback(int buttonNum) {
  // Button 1: toggle between SELECTING and current state
  if (buttonNum == 1) {
    if (stateManager->isSelecting()) {
      stateManager->enterHome();
    } else if (stateManager->isHome()) {
      if (!ensureMqttConnected()) {
        Serial.println("MQTT not connected - staying in home");
        tone(TONE_PIN, 200, 200);  // Error tone
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
    tone(TONE_PIN, 500, 100);  // Reset tone
  }
}

void connectToWiFi() {
  // Check WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    return;
  }

  String fv = WiFi.firmwareVersion();
  Serial.print("WiFi Module Firmware: ");
  Serial.println(fv);

  Serial.print("\nAttempting to connect to: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  trellisController->setKeyHandler(handleTrellisKey);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");

    // Success tone
    tone(TONE_PIN, 1047, 100);  // C6
    delay(150);
    tone(TONE_PIN, 1319, 100);  // E6
  } else {
    Serial.println("✗ Failed to connect to WiFi\n");
    // WiFi / MQTT

    ensureMqttConnected();
    // Failure tone
    tone(TONE_PIN, 200, 200);
  }
}

bool ensureMqttConnected() {
  if (mqttClient.connected()) {
    return true;
  }

  mqttClient.setServer(mqttBroker, mqttPort);

  Serial.print("Connecting to MQTT broker: ");
  Serial.print(mqttBroker);
  Serial.print(":");
  Serial.println(mqttPort);

  String clientId = "noodles-" + String(millis(), HEX);
  bool connected = mqttClient.connect(clientId.c_str());

  if (connected) {
    Serial.println("✓ MQTT connected");
  } else {
    Serial.print("✗ MQTT connect failed, rc=");
    Serial.println(mqttClient.state());
  }

  return connected;
}

void pollConnectivity() {
  unsigned long now = millis();
  if (now - lastConnectivityCheckMs < CONNECTIVITY_CHECK_INTERVAL_MS) {
    return;
  }
  lastConnectivityCheckMs = now;

  int wifiStatus = WiFi.status();
  if (wifiStatus != WL_CONNECTED) {
    Serial.print("WiFi down (status=");
    Serial.print(wifiStatus);
    Serial.println(") - retrying");

    WiFi.disconnect();
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 5) {
      delay(200);
      attempts++;
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✓ WiFi reconnected");
    } else {
      Serial.println("✗ WiFi still down");
      return;
    }
  }

  if (!mqttClient.connected()) {
    Serial.println("MQTT disconnected - attempting reconnect");
    ensureMqttConnected();
  }
}

void refreshSelectionPixels() {
  // Deprecated - StateManager now handles display updates
}

void updateSelectionBlink() {
  // Deprecated - StateManager now handles display updates
}

void enterHomeState() {
  // Deprecated - use stateManager->enterHome()
}

void enterSelectingState() {
  // Deprecated - use stateManager->enterSelecting()
}

void enterControlState() {
  // Deprecated - use stateManager->enterControl()
}

void handleTrellisKey(uint8_t key, bool pressed) {
  if (!pressed) return;

  if (stateManager->isSelecting() && key == 15) {  // APP_SELECT_KEY = 15
    stateManager->enterControl();
  }
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  Serial.println("\n\nNoodles Project");
  Serial.println("===============\n");

  // Initialize NeoTrellis controller
  trellisController = new NeoTrellisController(TONE_PIN);
  if (!trellisController->begin()) {
    Serial.println("Failed to initialize NeoTrellis controller!");
    while (1) delay(1);
  }
  trellisController->setKeyHandler(handleTrellisKey);

  // Initialize StateManager
  int appCount = sizeof(apps) / sizeof(AppDefinition);
  stateManager = new StateManager(trellisController, TONE_PIN, apps, appCount);

  // Initialize encoders
  encoder1 = new EncoderChannel(ENCODER1_PIN_A, ENCODER1_PIN_B, 1, TONE_PIN);
  encoder2 = new EncoderChannel(ENCODER2_PIN_A, ENCODER2_PIN_B, 2, TONE_PIN);

  encoder1->setCallback(encoderCallback);
  encoder2->setCallback(encoderCallback);

  // Initialize button controller
  simpleButtonPairController =
      new SimpleButtonPairController(BUTTON1_PIN, BUTTON2_PIN, TONE_PIN);
  simpleButtonPairController->begin();
  simpleButtonPairController->setCallback(buttonCallback);

  // WiFi / MQTT
  connectToWiFi();
  ensureMqttConnected();

  // Default state
  stateManager->enterHome();

  Serial.println("System ready!");
}

void loop() {
  trellisController->update();
  encoder1->update();
  encoder2->update();
  simpleButtonPairController->update();
  stateManager->updateBlink();
  pollConnectivity();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}
