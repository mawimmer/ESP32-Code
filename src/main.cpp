#include <Arduino.h>
#include <driver/pcnt.h>

#define CLK 2
#define DT 3
#define SW 4


unsigned long lastBlinkTime = 0;
bool ledState = LOW;



void setup() {
//Serial0 for reading content in VSC Terminal
    Serial0.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT); 
//Rotary Encoder Block - Start
    //Pin declarations
    pinMode(CLK, INPUT);
    pinMode(DT, INPUT);
    pinMode(SW, INPUT_PULLUP);
    //PCNT Initiation
//Rotary Encoder Block - End


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

