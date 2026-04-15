#include <Arduino.h>
#include <driver/pcnt.h>

#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21

unsigned long lastBlinkTime = 0;
bool ledState = LOW;


void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt){
    pcnt_config_t pcnt_config = {
        .counter_h_lim = PCNT_LIMIT_HIGH,
        .counter_l_lim = PCNT_LIMIT_LOW,
        

    };
}


void setup() {
//Serial0 for reading content in VSC Terminal
    Serial0.begin(115200);
    Serial0.println(String("ESP-IDF Version is: ") + esp_get_idf_version());
    pinMode(LED_BUILTIN, OUTPUT); 
}

void loop() {

    if (millis() - lastBlinkTime >= 500) {
        lastBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    
    // Only print "alive" when the LED toggles, so the terminal stays clean
        if (ledState == HIGH) {
            Serial0.println("ESP32 is alive!");
        }
    }

    //Rotary Encoder Block - Start

    

    //Rotary Encoder Block - End
}

