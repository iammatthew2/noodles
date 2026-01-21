/// @brief Handles WiFi connectivity and MQTT publish/subscribe communication.

#ifndef WIFI_MQTT_MANAGER_H
#define WIFI_MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiNINA.h>

class WiFiMQTTManager {
 public:
  WiFiMQTTManager(const char* ssid, const char* password,
                  const char* mqttBroker, uint16_t mqttPort);

  // Connection management
  void connect();
  void poll();

  // Status checks
  bool isWiFiConnected() const;
  bool isMqttConnected();

  // MQTT operations
  bool publish(const char* topic, const char* payload);

  // Get client for advanced usage
  PubSubClient& getMqttClient() { return mqttClient; }

 private:
  const char* ssid;
  const char* password;
  const char* mqttBroker;
  uint16_t mqttPort;

  WiFiClient wifiClient;
  PubSubClient mqttClient;

  unsigned long lastConnectivityCheckMs;
  static const unsigned long CONNECTIVITY_CHECK_INTERVAL_MS = 5000;
  unsigned long lastBrokerPingLogMs;
  static const unsigned long BROKER_PING_LOG_INTERVAL_MS = 10000;

  bool ensureMqttConnected();
  void pollConnectivity();
  void logBrokerPing();
};

#endif
