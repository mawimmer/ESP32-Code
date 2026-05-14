#include <Arduino.h>
#include <wledBridge.h>

#if defined(WOKWI_SIM) || defined (MOCK_COMPILE)
    #define Serial Serial0
    #include <wled_mock.h>
#elif defined(WLED_DEV)
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

Strip strip;
PinManagerClass pinManager;

void setup() {
    
    Serial.begin(115200);
    delay(1000);

    Serial.println("--- Start WLED Usermod Mock ---");

    //Instance_encoderManager.initOLED();

    Instance_wledBridge.setup();

    Serial.println("[Mock] Usermod active.");

}

void loop() {
    
    Instance_wledBridge.loop();

#ifdef WOKWI_SIM
    vTaskDelay(pdMS_TO_TICKS(10));
#endif
}
