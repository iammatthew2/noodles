#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiNINA.h>

#include "EncoderChannel.h"
#include "InputHandler.h"
#include "NeoTrellisController.h"
#include "SimpleButtonPairController.h"
#include "StateManager.h"
#include "WiFiMQTTManager.h"
#include "secrets.h"

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// App definitions
AppDefinition apps[] = {
    {"Yodel", "apps/yodel/control", 255, 80, 80},
    {"Skippy", "apps/skippy/control", 80, 255, 120},
    {"Jibbers", "apps/jibbers/control", 80, 180, 255},
    {"Pickles", "apps/pickles/control", 255, 200, 80},
    {"Puddles", "apps/puddles/control", 200, 120, 255},
    {"Nurbo", "apps/nurbo/control", 255, 140, 50},
};

StateManager* stateManager;
WiFiMQTTManager* wifiMqttManager;
InputHandler* inputHandler;

// Forward declarations
void handleTrellisKey(uint8_t key, bool pressed);

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
  inputHandler->handleEncoderCallback(channel, direction);
}

void buttonCallback(int buttonNum) {
  inputHandler->handleButtonCallback(buttonNum);
}

void connectToWiFi() { wifiMqttManager->connect(); }

void runStartupRainbowBurst() {
  const unsigned long stepDelayMs = 60;

  for (uint8_t step = 0; step < 16; step++) {
    for (uint8_t i = 0; i < 16; i++) {
      uint8_t hue = (uint8_t)((i * 16 + step * 16) & 0xFF);
      uint8_t r, g, b;

      if (hue < 85) {
        r = hue * 3;
        g = 255 - hue * 3;
        b = 0;
      } else if (hue < 170) {
        uint8_t p = hue - 85;
        r = 255 - p * 3;
        g = 0;
        b = p * 3;
      } else {
        uint8_t p = hue - 170;
        r = 0;
        g = p * 3;
        b = 255 - p * 3;
      }

      trellisController->setPixelColor(i, trellisController->color(r, g, b));
    }
    trellisController->showPixels();
    delay(stepDelayMs);
  }

  tone(TONE_PIN, 1047, 80);
  delay(100);
  tone(TONE_PIN, 1319, 80);
}

void refreshSelectionPixels() {}

void updateSelectionBlink() {}

void enterHomeState() {}

void enterSelectingState() {}

void enterControlState() {}

void handleTrellisKey(uint8_t key, bool pressed) {
  inputHandler->handleTrellisKey(key, pressed);
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

  // Initialize WiFi/MQTT Manager
  wifiMqttManager = new WiFiMQTTManager(ssid, password, MQTT_BROKER, MQTT_PORT);

  // Initialize InputHandler
  inputHandler = new InputHandler(stateManager, wifiMqttManager, TONE_PIN);
  inputHandler->setNeoTrellis(trellisController);

  // Initialize encoders
  encoder1 = new EncoderChannel(ENCODER1_PIN_A, ENCODER1_PIN_B, 1);
  encoder2 = new EncoderChannel(ENCODER2_PIN_A, ENCODER2_PIN_B, 2);

  encoder1->setCallback(encoderCallback);
  encoder2->setCallback(encoderCallback);

  // Initialize button controller
  simpleButtonPairController =
      new SimpleButtonPairController(BUTTON1_PIN, BUTTON2_PIN, TONE_PIN);
  simpleButtonPairController->begin();
  simpleButtonPairController->setCallback(buttonCallback);

  // WiFi / MQTT
  wifiMqttManager->connect();

  // Default state
  stateManager->enterHome();

  // One-time startup rainbow burst
  runStartupRainbowBurst();

  // Return to HOME display after burst
  stateManager->enterHome();

  Serial.println("System ready!");
}

void loop() {
  trellisController->update();
  encoder1->update();
  encoder2->update();
  simpleButtonPairController->update();
  stateManager->updateBlink();
  wifiMqttManager->poll();

  if (wifiMqttManager->isMqttConnected()) {
    wifiMqttManager->getMqttClient().loop();
  }
}
