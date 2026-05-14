#pragma once
#include <Arduino.h>
#include <driver/pcnt.h>

#if defined(WOKWI_SIM) || defined (MOCK_COMPILE)
    #define Serial Serial0
    #include <wled_mock.h>
#elif defined(WLED_DEV)
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

void IRAM_ATTR buttonISR(void* arg);

class rotaryEncoder {

    public:

        /**
     * Hardware Info
     */
    pcnt_unit_t unit;
    gpio_num_t pin_clk;
    gpio_num_t pin_dt;
    gpio_num_t pin_sw;

    //int effectValue = 0;
    //int8_t segmentID;

    int pulsesPerDetent = 2;

    int rotationDelay = 0;

    friend void IRAM_ATTR buttonISR(void* arg);
        /**
     * Click Modi
     */


    enum class ButtonEventType {
        NONE,
        SHORT_PRESS,
        DOUBLE_PRESS,
        LONG_PRESS,
    };


        /**
     * Click and Rotation Execution States
     */
    ButtonEventType eventButton = ButtonEventType::NONE;

    bool eventRotation = false;
    int16_t deltaValue = 0;

    rotaryEncoder() {}
    rotaryEncoder(pcnt_unit_t u, int clk, int dt, int sw) : 
        unit(u), pin_clk(static_cast<gpio_num_t>(clk)), pin_dt(static_cast<gpio_num_t>(dt)), pin_sw(static_cast<gpio_num_t>(sw)) {};

    private:

        bool stateChanged = false;

        int longShortPressThreshold = 500;
        int doublePressThreshold = 200;
        int BRIGHTNESS_ROTATION_DELAY = 40;
        int EFFECT_ROTATION_DELAY = 150;

        /**
         * Rotation Variables
         */
        unsigned long timeOfLastRotation = 0;
        bool rotationPending = false;

        /**
         * Click Variables
         */
        volatile uint32_t timeOfLastClick = 0;
        volatile uint32_t lastEdge = 0;
        volatile bool buttonIsPressed = false;
        volatile bool buttonWasPressed = false;
        volatile bool buttonPressHandled = true;
        bool waitingForDoubleClick = false;
        u_int32_t timeOfFirstClick = 0;

        public:

        void updatePCNT_Unit(int i);

        void updateButtonState(int32_t timeNOW);
        

};
