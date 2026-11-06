#define LED_WHITE 33
#define LED_RED   32

void setup() {
  Serial.begin(115200);
  pinMode(LED_WHITE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
}

void loop() {
  Serial.println("WHITE on");
  digitalWrite(LED_WHITE, HIGH);
  delay(1000);
  digitalWrite(LED_WHITE, LOW);

  Serial.println("RED on");
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_RED, LOW);
}