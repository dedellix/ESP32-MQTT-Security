#define LED_WHITE  33
#define LED_RED    32
#define BUTTON_PIN 27

bool ledState = false;
int lastButton = LOW;
unsigned long lastChange = 0;
const unsigned long DEBOUNCE_MS = 50;

void setup() {
  pinMode(LED_WHITE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);   // external pull-down
  digitalWrite(LED_WHITE, LOW);
}

void loop() {
  int button = digitalRead(BUTTON_PIN);

  // Act only on a new press (LOW -> HIGH), ignoring bounces
  if (button == HIGH && lastButton == LOW && millis() - lastChange > DEBOUNCE_MS) {
    ledState = !ledState;
    digitalWrite(LED_WHITE, ledState);
    lastChange = millis();
  }

  lastButton = button;
}