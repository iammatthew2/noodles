#include "WiFiMQTTManager.h"

WiFiMQTTManager::WiFiMQTTManager(const char* ssid, const char* password,
                                 const char* mqttBroker, uint16_t mqttPort)
    : ssid(ssid),
      password(password),
      mqttBroker(mqttBroker),
      mqttPort(mqttPort),
      mqttClient(wifiClient),
      lastConnectivityCheckMs(0) {}

void WiFiMQTTManager::connect() {
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
    tone(7, 1047, 100);  // C6
    delay(150);
    tone(7, 1319, 100);  // E6
  } else {
    Serial.println("✗ Failed to connect to WiFi\n");
    ensureMqttConnected();
    // Failure tone
    tone(7, 200, 200);
  }
}

bool WiFiMQTTManager::isWiFiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WiFiMQTTManager::isMqttConnected() { return mqttClient.connected(); }

bool WiFiMQTTManager::ensureMqttConnected() {
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

void WiFiMQTTManager::pollConnectivity() {
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

void WiFiMQTTManager::poll() {
  pollConnectivity();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}

bool WiFiMQTTManager::publish(const char* topic, const char* payload) {
  if (!mqttClient.connected()) {
    return false;
  }
  return mqttClient.publish(topic, payload);
}
