#include <Arduino.h>

#include "wled.h"
#include "Multiple_Rotary_Encoder.h"

Strip strip;
PinManagerClass pinManager;

void setup() {
    
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Starte WLED Usermod Mock ---");

    multiple_rotary_encoder.setup();


    multiple_rotary_encoder.enable(true);
    Serial.println("[Mock] Usermod wurde manuell aktiviert.");

}

void loop() {
    
    multiple_rotary_encoder.loop();

    vTaskDelay(pdMS_TO_TICKS(10));
}
