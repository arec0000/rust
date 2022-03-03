#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  pinMode(21, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  digitalWrite(21, HIGH);
  delay(1000);
  digitalWrite(21, LOW);
  delay(1000);
  Serial.println("aaaa");
}