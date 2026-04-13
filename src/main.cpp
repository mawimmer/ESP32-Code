#include <Arduino.h>

// Beim Nano ESP32 ist die LED oft an einem speziellen Pin
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); 
}

void loop() {
  Serial.println("Der ESP32 lebt! Signal an Kessel...");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}