#include <Arduino.h>
#include <WiFiNINA.h>

#include "EncoderChannel.h"
#include "NeoTrellisController.h"
#include "SimpleButtonPairController.h"
#include "secrets.h"

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

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

void encoderCallback(int channel, int direction) {
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
  Serial.print("Button ");
  Serial.print(buttonNum);
  Serial.println(" callback triggered");
  // Additional logic can be added here
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

    // Failure tone
    tone(TONE_PIN, 200, 200);
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

  Serial.println("System ready!");
}

void loop() {
  trellisController->update();
  encoder1->update();
  encoder2->update();
  simpleButtonPairController->update();
}
