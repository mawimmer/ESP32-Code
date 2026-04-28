#include <Arduino.h>

#include "wled.h"
#include "multiple_rotary_encoder.h"

Strip strip;
PinManagerClass pinManager;

void setup() {
    
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Starte WLED Usermod Mock ---");

    encoder_manager.initOLED();

    encoder_manager.setup();


    encoder_manager.enable(true);
    Serial.println("[Mock] Usermod wurde manuell aktiviert.");

}

void loop() {
    
    encoder_manager.loop();

    vTaskDelay(pdMS_TO_TICKS(10));
}
