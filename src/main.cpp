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
  wifiMqttManager->connect();

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
  wifiMqttManager->poll();

  if (wifiMqttManager->isMqttConnected()) {
    wifiMqttManager->getMqttClient().loop();
  }
}
