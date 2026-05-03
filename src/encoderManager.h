#include <Arduino.h>
#include <driver/pcnt.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <rotaryEncoder.h>
#include <functional>

#if defined(WOKWI_SIM)
  #define Serial Serial0
#endif

/**
 * Rotary Encoder Specifications
 */
#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21

const int MAX_ENCODERS = 4;

extern int NUM_ENCODERS;



//Config Variables



/**
 * Configures and Sets Up a PCNT unit
 */
//inline void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt);

typedef std::function<void(int encoderIndex, rotaryEncoder::ButtonEventType event, int16_t delta)> EncoderEventCallback;

class encoderManager {

private:

    int longShortPressThreshold = 500;
    int doublePressThreshold = 400;
    bool enabled = false;

    // Rotary Encoders Pin Declarations (actual ESP32-S3 Pinout Numbers)
    rotaryEncoder Encoders[MAX_ENCODERS]{
        {PCNT_UNIT_0, 5, 6, 7},
        {PCNT_UNIT_1, -1, -1, -1},
        {PCNT_UNIT_2, -1, -1, -1},
        {PCNT_UNIT_3, -1, -1, -1}
    };

    EncoderEventCallback onEventTriggered = nullptr;

    void init_PCNT_UNITS();

    void updateHardware();

    void global_EventHandler();

public:

    int BRIGHTNESS_ROTATION_DELAY = 40;
    int EFFECT_ROTATION_DELAY = 150;

    //const int longShortPressThreshold = 500;
    bool global_eventPending = false;

    inline void enable(bool e) { enabled = e; }

    inline bool isEnabled() { return enabled; }

    void setEventHandler(EncoderEventCallback handler) {
        onEventTriggered = handler;
    }

    void setup();

    void loop();

    void setEncoderPins(int index, int clk, int dt, int sw);

    void setThresholds(int index, int longPress, int doublePress);

};

extern encoderManager Instance_encoderManager;

