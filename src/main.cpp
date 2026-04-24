#include <Arduino.h>

// Hier holst du deine Usermod-Datei rein!
#include "Multiple_Rotary_Encoder.h"

void setup() {
    Serial.begin(115200);
    // Usermod starten
    multiple_rotary_encoder.setup();
}

void loop() {
    // Usermod endlos laufen lassen
    multiple_rotary_encoder.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
}
