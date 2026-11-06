// #include <ESPPubSubClientWrapper.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ---- CONFIG ----
const char* WIFI_SSID   = "WIFI_SSID";  // Replace your WiFi SSID name
const char* WIFI_PASS   = "WIFI_PASSWORD";    // Replace your WiFi password
const char* MQTT_SERVER = "172.x.x.x";   // Replace your MQTT server IP (usually your laptop's IP)
const int   MQTT_PORT   = 1883;
const char* DEVICE_ID   = "esp32_1";        // "esp32_2" on the second board
const char* TOPIC_ALARM = "esp32/alarm/humidity";


#define DHT_PIN    26
#define DHT_TYPE   DHT11
#define BUTTON_PIN 27
#define LED_RED 32



void onMessage(char* topic, byte* payload, unsigned int len) {
  if (len > 0 && strcmp(topic, TOPIC_ALARM) == 0) {
    digitalWrite(LED_RED, payload[0] == '1' ? HIGH : LOW);
  }
}

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char tTemp[48], tHum[48];
unsigned long lastRead = 0;
int lastButton = LOW;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\nConnected! ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT... ");
    if (mqtt.connect(DEVICE_ID)) {
      Serial.println("OK");
      mqtt.subscribe(TOPIC_ALARM);
    } else {
      Serial.printf("failed (rc=%d), retry in 2s\n", mqtt.state());
      delay(2000);
    }
  }
}

void readAndPublish() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("DHT read failed");
    return;
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f", t);
  mqtt.publish(tTemp, buf);
  snprintf(buf, sizeof(buf), "%.1f", h);
  mqtt.publish(tHum, buf);
  Serial.printf("Published T=%.1f H=%.1f\n", t, h);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_RED, OUTPUT);
  dht.begin();

  snprintf(tTemp, sizeof(tTemp), "esp32/%s/temperature", DEVICE_ID);
  snprintf(tHum,  sizeof(tHum),  "esp32/%s/humidity",    DEVICE_ID);

  connectWiFi();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(onMessage);   // after mqtt.setServer(...)
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  int button = digitalRead(BUTTON_PIN);
  if (button == HIGH && lastButton == LOW && millis() - lastRead > 1000) {
    lastRead = millis();
    Serial.print("[button] ");
    readAndPublish();
  }
  lastButton = button;

  if (millis() - lastRead >= 5000) {   // every 5 seconds
    lastRead = millis();
    readAndPublish();
  }
}