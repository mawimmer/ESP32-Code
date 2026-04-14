#include <Arduino.h>

void setup() {
    //Serial0 for reading content in VSC Terminal
  Serial0.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); 
}

void loop() {
  Serial0.println("ESP32 is alive!");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}

