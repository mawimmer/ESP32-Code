#include <Arduino.h>
#include <multiple_rotary_encoder.h>
#ifdef WOKWI_SIM
    #define Serial Serial0
    #include <wled.h>
#else
    #include <wled.h>
#endif

Strip strip;
PinManagerClass pinManager;

void setup() {
    
    Serial.begin(115200);
    delay(1000);

    Serial.println("--- Start WLED Usermod Mock ---");

    encoder_manager.initOLED();

    encoder_manager.setup();


    encoder_manager.enable(true);
    Serial.println("[Mock] Usermod active.");

}

void loop() {
    
    encoder_manager.loop();

#ifdef WOKWI_SIM
    vTaskDelay(pdMS_TO_TICKS(10));
#endif
}
