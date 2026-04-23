#pragma once
#include <Arduino.h>
#include <driver/pcnt.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1 // Reset-Pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/**
 * prints basic description on screen
 */
void initDisplay () {
    Wire.begin();
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
      Serial.println(F("SSD1306 Fehler: Display nicht gefunden!"));
    }
    // 1. clear display buffer
    display.clearDisplay();
    // 2. set fontsize
    display.setTextSize(1);
    // 3. set fontcolor
    display.setTextColor(SSD1306_WHITE);
    // 4. set starting point
    display.setCursor(0, 10);
    display.println(F("Brightness:"));
    display.setCursor(0, 20);
    display.println(F("Effect:"));
    // 6. transfer buffer to display
    display.display();
};

void init_PCNT_UNITS () {

    for (int i = 0 ; i < NUM_ENCODERS ; i++) {

        // Referencing Encoder to Encoder[i] Pointer in the Setup Scope
        RotaryEncoder& Encoder = Encoders[i];

        gpio_set_direction(Encoder.pin_clk, GPIO_MODE_INPUT);
        gpio_set_pull_mode(Encoder.pin_clk, GPIO_PULLUP_ONLY);
        gpio_set_direction(Encoder.pin_dt, GPIO_MODE_INPUT);
        gpio_set_pull_mode(Encoder.pin_dt, GPIO_PULLUP_ONLY);
        gpio_set_direction(Encoder.pin_sw, GPIO_MODE_INPUT);
        gpio_set_pull_mode(Encoder.pin_sw, GPIO_PULLUP_ONLY);

        gpio_set_intr_type(Encoder.pin_sw, GPIO_INTR_ANYEDGE);
        gpio_isr_handler_add(Encoder.pin_sw, buttonISR, &Encoder);

        setup_PCNT_UNIT(Encoder.unit, Encoder.pin_clk, Encoder.pin_dt );

        Serial.printf("Encoder # %d has been initialized! \r\n", i );
    };

};