#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "encoderTypes.h" // Needs to know the modes to draw them!

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

class DisplayManager {
private:
    Adafruit_SSD1306 display;
    bool isInitialized = false;

public:
    DisplayManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

    void setup(int pinSDA, int pinSCL) {
        Wire.begin(pinSDA, pinSCL);
        
        // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
            Serial.println(F("SSD1306 allocation failed"));
            return;
        }
        
        isInitialized = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("WLED Encoders");
        display.display();
    }

    // A clean, generic function for the Bridge to call
    void drawEncoderState(int encoderIndex, int8_t segmentID, Rotary_Encoder_MODI mode, selectedEffectConfig config, int16_t value, const char* effectName = nullptr) {
        if (!isInitialized) return;

        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(1);

        // 1. Draw Header
        display.printf("Encoder %d -> Seg %d\n", encoderIndex, segmentID);
        display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

        // 2. Draw Mode & Values
        display.setCursor(0, 15);
        display.setTextSize(2);

        switch(mode) {
            case Rotary_Encoder_MODI::TOGGLED_OFF:
                display.println("OFF");
                break;
                
            case Rotary_Encoder_MODI::BRIGHTNESS_MODI:
                display.println("BRIGHT:");
                display.setCursor(0, 35);
                display.printf("%d/255\n", value); // Draw the absolute brightness
                break;
                
            case Rotary_Encoder_MODI::EFFECT_MODI:
                display.println("EFFECT:");
                display.setCursor(0, 35);
                if (effectName != nullptr && effectName[0] != '\0') {
                    display.printf("%s\n", effectName); 
                } else {
                    display.printf("ID: %d\n", value); // Fallback just in case
                }
                break;   
            case Rotary_Encoder_MODI::EFFECT_CONFIG_MODI:
            case Rotary_Encoder_MODI::EFFECT_CONFIG_SELECTED_MODI:
                display.setTextSize(1);
                display.print("CONFIG:");
                
                display.setCursor(0, 25);
                if (mode == Rotary_Encoder_MODI::EFFECT_CONFIG_MODI) display.print("> ");
                else display.print("  ");

                // Print the dynamic name we fetched from the WLED Bridge!
                if (effectName != nullptr && effectName[0] != '\0') {
                    display.println(effectName); 
                } else {
                    display.println("PARAMETER"); // Fallback
                }
                
                display.setCursor(0, 40);
                display.setTextSize(2);
                if (mode == Rotary_Encoder_MODI::EFFECT_CONFIG_SELECTED_MODI) display.print("> ");
                display.printf("%d\n", value);
                break;
        }

        display.display();
    }

};
