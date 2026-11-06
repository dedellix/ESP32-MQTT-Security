#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID   = "WIFI_SSID";  // Replace your WiFi SSID name
const char* WIFI_PASS   = "WIFI_PASSWORD";    // Replace your WiFi password
const char* MQTT_SERVER = "172.x.x.x";   // Replace your MQTT server IP (usually your laptop's IP)
const char* DEVICE_ID   = "esp32_2";

const char* TOPIC_ALARM = "esp32/esp32_2/alarm";
const char* TOPIC_ACK   = "esp32/esp32_2/ack";

#define LED_WHITE  33
#define LED_RED    32
#define BUTTON_PIN 27            // external pull-down: pressed = HIGH

const unsigned long BLINK_MS = 300;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool alarmOn = false;            // server says humidity is too high
bool muted = false;              // user pressed the button
bool redPhase = false;
int lastButton = LOW;
unsigned long lastBlink = 0, lastPress = 0, lastTry = 0;

void setLeds(bool red, bool white) {
  digitalWrite(LED_RED, red);
  digitalWrite(LED_WHITE, white);
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  if (strcmp(topic, TOPIC_ALARM) != 0 || len == 0) return;
  if (payload[0] == '1') {
    alarmOn = true;              // if already muted, it stays muted
  } else {
    alarmOn = false;
    muted = false;               // alarm cleared: re-arm
    setLeds(false, false);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_WHITE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  setLeds(false, false);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\nConnected, IP: %s\n", WiFi.localIP().toString().c_str());

  mqtt.setServer(MQTT_SERVER, 1883);
  mqtt.setCallback(onMessage);
}

void loop() {
  // Non-blocking MQTT reconnect, so the button and LEDs keep working
  if (!mqtt.connected()) {
    if (millis() - lastTry > 2000) {
      lastTry = millis();
      if (mqtt.connect(DEVICE_ID)) {
        mqtt.subscribe(TOPIC_ALARM);
        Serial.println("MQTT connected");
      } else {
        Serial.printf("MQTT failed, rc=%d\n", mqtt.state());
      }
    }
  } else {
    mqtt.loop();
  }

  // Button: mute the alarm
  int button = digitalRead(BUTTON_PIN);
  if (button == HIGH && lastButton == LOW && millis() - lastPress > 50) {
    lastPress = millis();
    if (alarmOn && !muted) {
      muted = true;
      setLeds(false, false);
      mqtt.publish(TOPIC_ACK, "1");
      Serial.println("Alarm muted");
    }
  }
  lastButton = button;

  // Alternate red / white while the alarm is on and not muted
  if (alarmOn && !muted && millis() - lastBlink >= BLINK_MS) {
    lastBlink = millis();
    redPhase = !redPhase;
    setLeds(redPhase, !redPhase);
  }
}