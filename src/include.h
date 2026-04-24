#pragma once
#include <Arduino.h>
#include <driver/pcnt.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1

/**
 * Rotary Encoder Specifications
 */

#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21
#define ROTARY_ENCODER_DETENTS 30

const int NUM_ENCODERS = 1;

    /**
     * Click Modi
     */

    enum Rotary_Encoder_MODI {
        BRIGHTNESS_MODI,
        EFFECT_MODI,
        TOGGLED_OFF
    };

    /**
     * Button State
     */
    enum eventButton {
        NONE,
        SHORT_PRESS,
        LONG_PRESS
    };


    /**
     * Struct Holding Information of each Encoder
     */
    struct RotaryEncoder{

        /**
         * Hardware Info
         * Unit#; GPIO-PIN# CLK; GPIO-PIN#-DT; GPIO-PIN#-SW
         */
        pcnt_unit_t unit;
        gpio_num_t pin_clk;
        gpio_num_t pin_dt;
        gpio_num_t pin_sw;

        /**
         * Rotation Variables
         */
        int16_t deltaValue = 0;
        unsigned long TimeOfLastRotation = 0;
        bool rotationPending = false;
        int rotationDelay = 0;

        /**
         * Click Variables
         */
        volatile unsigned long TimeOfLastClick = 0;
        volatile unsigned long lastEdge = 0;
        volatile bool buttonIsPressed = false;
        volatile bool buttonWasPressed = false;
        volatile bool buttonPressHandled = true;

        /**
         * Click and Rotation Execution States
         */
        enum eventButton eventButton = NONE;

        bool eventRotation = false;
        enum Rotary_Encoder_MODI MODI = TOGGLED_OFF;

        /**
         * Brightness and Effect Variables
         */
        int brightness = 0;
        int effect = 0;

        RotaryEncoder(pcnt_unit_t u, int clk, int dt, int sw):
        unit(u), pin_clk(static_cast<gpio_num_t>(clk)), pin_dt(static_cast<gpio_num_t>(dt)), pin_sw(static_cast<gpio_num_t>(sw)) {};
    };


    /**
     * Configures and Sets Up a PCNT unit
     * 
     * @param unit the unit which is initialized
     * @param pin_clk the GPIO port for pulse
     * @param pin_dt the GPIO port for ctrl
     * 
     * @return void
     */
    void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt){

        pcnt_config_t pcnt_config = {
            .pulse_gpio_num = pin_clk,
            .ctrl_gpio_num = pin_dt,
            .lctrl_mode = PCNT_MODE_REVERSE,
            .hctrl_mode = PCNT_MODE_KEEP,
            .pos_mode = PCNT_COUNT_INC,
            .neg_mode = PCNT_COUNT_DEC,
            .counter_h_lim = PCNT_LIMIT_HIGH,
            .counter_l_lim = PCNT_LIMIT_LOW,
            .unit = unit,
            .channel = PCNT_CHANNEL_0,
        };

        pcnt_unit_config(&pcnt_config);

        pcnt_set_filter_value(unit, 1000);

        pcnt_filter_enable(unit);

        pcnt_counter_pause(unit);
        pcnt_counter_clear(unit);
        //pcnt_counter_resume(unit); // Is handled by switch case

    };
    
        /**
     * Hardware Interrupt Function
     * This function is executed when a Rising or Falling Edge is detected
     */
    void IRAM_ATTR buttonISR(void* arg) {
        // Referencing Encoder to Pointer in the ISR Scope
        RotaryEncoder& Encoder = *static_cast<RotaryEncoder*>(arg);

        if(millis() - Encoder.lastEdge >= 50) {

            if(gpio_get_level(Encoder.pin_sw) == LOW) {

                // prevents bouncing
                Encoder.lastEdge = millis();

                // gets time of last click to determine long vs short click
                Encoder.TimeOfLastClick = Encoder.lastEdge;

                // sets button is pressed until rising edge neglects it
                Encoder.buttonIsPressed = true;
                Encoder.buttonPressHandled = false;

                Encoder.buttonWasPressed = false;

            } else {
                // prevents bouncing        
                Encoder.lastEdge = millis();

                // shifts buttonIsPressed to buttonWasPressed
                Encoder.buttonIsPressed = false;
                Encoder.buttonWasPressed = true;

            };

        };

    };