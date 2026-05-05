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

void animate() {
    int progress = 0;

    while (true) {
        delay(40);

        float norm = (float)progress / 100.0f; // normalize to 0..1
        float factor = 0.1f + 1.0f * norm * norm;
        int step = 1 + (int)(factor * 5);

        progress += step;

        display.clearDisplay();
        constrain(progress, 0, 100);
        int degree = map(progress,0, 100, 25, 300);

        int cx = 32;
        int cy = 32;
        int length = 25;

        float rad = degree * 3.14159 / 180.0;

        int x2 = cx + length * sin(rad);
        int y2 = cy - length * cos(rad);

        display.clearDisplay();
        display.drawLine(cx, cy, x2, y2, SSD1306_WHITE);
        display.drawCircle(32,32,27,SSD1306_WHITE);
        display.display();

        // // thick bar
        // display.fillRoundRect(5, 10, progress, 5, 3, SSD1306_WHITE);
        // // outer layer thick bar
        // display.drawRoundRect(3, 8, 102, 9, 3, SSD1306_WHITE);
        // display.setCursor(105, 9);
        // display.setTextSize(1);
        // display.printf("%d%%", progress);
        // // // line with dot
        // display.drawLine(0, 25, 100, 25, SSD1306_WHITE);
        // display.fillCircle(progress,25,4,SSD1306_BLACK);
        // display.fillCircle(progress,25,3,SSD1306_WHITE);
        // display.setCursor(105, 24);
        // display.setTextSize(1);
        // display.printf("%d%%", progress);


        // dots
        // for (int i = 0; i < 100; i += 7) {
        //     display.fillCircle(i + 5, 40, 2, SSD1306_WHITE);
        // }
        // display.fillRect(progress, 35, 128 - progress, 10, SSD1306_BLACK);

        // pills
        // for (int i = 0; i < 100; i += 15) {
        //     display.fillRoundRect(i , 50, 15, 6, 3, SSD1306_WHITE);
        // }
        // display.fillRect(progress, 50, 128 - progress, 10, SSD1306_BLACK);
        // // outer layer pills
        // for (int i = 0; i < 100; i += 15) {
        //     display.drawRoundRect(i - 1 , 50, 15, 6, 3, SSD1306_WHITE);
        // }

        // rectangle
        // for (int i = 0; i < 100; i++) {
        //     int y = map(i, 0, 100, 1, 32);
        //     display.fillRect(i, 64 - y, 1, y, SSD1306_WHITE);
        // }
        // display.fillRect(progress, 0, 128 - progress, 64, SSD1306_BLACK);
        display.display();



        //progress++;

        if (progress > 100) {
            progress = 0;
        }
    }
}

void plotCurve() {
    display.clearDisplay();

    // OLED is 128x64
    // We'll use:
    // X = 0 → 127
    // Y = 63 → 0 (screen is inverted vertically)

    for (int x = 0; x < 128; x++) {
        // Map screen x to opacity 0..255
        int opacity = map(x, 0, 127, 1, 255);

        // Same formula as your brightness code
        float norm = (float)opacity / 255.0f;
        float factor = 0.1f + 3.0f * norm * norm;
        int step = 1 + (int)(factor * 20);

        // step is roughly 3..33
        // Map step to screen height
        int y = map(step, 1, 35, 63, 0);

        display.drawPixel(x, y, SSD1306_WHITE);
    }

    // Optional axes
    display.drawLine(0, 63, 127, 63, SSD1306_WHITE); // X axis
    display.drawLine(0, 0, 0, 63, SSD1306_WHITE);    // Y axis

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
                //display.println("OFF");
                animate ();
                //plotCurve ();
                break;
                
            case Rotary_Encoder_MODI::BRIGHTNESS_MODI: {
                // display.println("BRIGHT:");
                // display.setCursor(0, 35);
                // display.printf("%d/255\n", value); // Draw the absolute brightness
                int barWidth = map(value, 0, 255, 0, 100);
                display.fillRect(0, 45, barWidth, 1, SSD1306_WHITE);
                display.fillCircle(barWidth, 45, 5, SSD1306_BLACK);
                display.fillCircle(barWidth, 45, 3, SSD1306_WHITE);
                display.setCursor(103, 45);
                display.setTextSize(1);
                display.printf("%% %d\n", barWidth);
                break;
                
            } 
            case Rotary_Encoder_MODI::EFFECT_MODI:
                display.println("EFFECT:");
                display.setCursor(0, 35);
                if (effectName != nullptr && effectName[0] != '\0') {
                    display.printf("%s\n", effectName); 
                } else {
                    display.printf("ID: %d\n", value); // Fallback just in case
                }
                break;   
            case Rotary_Encoder_MODI::EFFECT_SELECT_SLIDERS_MODI:
            case Rotary_Encoder_MODI::EFFECT_ADJUST_SLIDERS_MODI:
                display.setTextSize(1);
                display.print("CONFIG:");
                
                display.setCursor(0, 25);
                if (mode == Rotary_Encoder_MODI::EFFECT_SELECT_SLIDERS_MODI) display.print("> ");
                else display.print("  ");

                // Print the dynamic name we fetched from the WLED Bridge!
                if (effectName != nullptr && effectName[0] != '\0') {
                    display.println(effectName); 
                } else {
                    display.println("PARAMETER"); // Fallback
                }
                
                display.setCursor(0, 40);
                display.setTextSize(2);
                if (mode == Rotary_Encoder_MODI::EFFECT_ADJUST_SLIDERS_MODI) display.print("> ");
                display.printf("%d\n", value);
                break;
        }

        display.display();
    }

};
