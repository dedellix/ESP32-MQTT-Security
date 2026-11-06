#include "DHT.h"

#define DHT_PIN    26
#define DHT_TYPE   DHT11
#define LED_WHITE  33
#define BUTTON_PIN 27

DHT dht(DHT_PIN, DHT_TYPE);

bool ledState = false;
int lastButton = LOW;
unsigned long lastChange = 0;
unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_WHITE, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  dht.begin();
}

void loop() {
  // Toggle button (unchanged)
  int button = digitalRead(BUTTON_PIN);
  if (button == HIGH && lastButton == LOW && millis() - lastChange > 50) {
    ledState = !ledState;
    digitalWrite(LED_WHITE, ledState);
    lastChange = millis();
  }
  lastButton = button;

  // DHT11 every 2 seconds (it cannot be read faster)
  if (millis() - lastRead >= 2000) {
    lastRead = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("DHT read failed");
    } else {
      Serial.printf("Temperature: %.1f C   Humidity: %.1f %%\n", t, h);
    }
  }
}