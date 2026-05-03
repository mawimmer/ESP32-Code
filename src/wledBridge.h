#pragma once
#include <Arduino.h>
#include "displayManager.h"

#if defined(WOKWI_SIM) || defined (MOCK_COMPILE)
    #define Serial Serial0
    #include <wled_mock.h>
#elif defined(WLED_DEV)
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

#pragma once

#if defined(WOKWI_SIM) || defined (MOCK_COMPILE)
    #define Serial Serial0
    #include <wled_mock.h>
#elif defined(WLED_DEV)
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

#include "encoderTypes.h"
#include "encoderManager.h"

static const char _name[] PROGMEM = "Multiple Rotary Encoder";
static const char _enabled[] PROGMEM = "Enabled";
static const char _pinCLK[] PROGMEM = "Pin CLK";
static const char _pinDT[] PROGMEM = "Pin DT";
static const char _pinSW[] PROGMEM = "Pin SW";
static const char _segID[] PROGMEM = "Segment ID";
static const char _longShortPressThreshold[] PROGMEM = "Long Press Threshold";
static const char _doublePressThreshold[] PROGMEM = "Double Press Threshold";
static const char _displayPinSDA[] PROGMEM = "Display PIN SDA";
static const char _displayPinSCL[] PROGMEM = "Display PIN SCL";

class WLED_Bridge : public Usermod {
private:
    // 1. Instantiate your pure hardware manager
    encoderManager hardwareManager;
    DisplayManager oledDisplay;

    // 2. Single Source of Truth
    Rotary_Encoder_MODI encoderModes[4];
    selectedEffectConfig encoderConfigs[4];
    int8_t encoderSegments[4];

    // --- CONFIG VARIABLES ---
    bool enabled = true;
    int numEncoders = 1;
    int longPressThreshold = 500;
    int doublePressThreshold = 400;
    int displayPinSDA = 21; // Or whatever your default is
    int displayPinSCL = 22;

    int8_t pinCLK[4] = {5, -1, -1, -1};
    int8_t pinDT[4]  = {6, -1, -1, -1};
    int8_t pinSW[4]  = {7, -1, -1, -1};

    // --- WLED HELPER FUNCTIONS (Absorbed from your old namespace) ---
    void toggleSegment(int8_t segmentID) {
        Segment& seg = strip.getSegment(segmentID);

        seg.setOption(SEG_OPTION_ON, !seg.getOption(SEG_OPTION_ON));
        stateUpdated(CALL_MODE_BUTTON);
    }

    void setSegmentBrightness(int8_t segmentID, int delta) {
        Segment& seg = strip.getSegment(segmentID);
        // Add your math/scaling logic here!
        seg.opacity = constrain(seg.opacity + delta, 1, 255);
        stateUpdated(CALL_MODE_BUTTON);
    }

    void setSegmentEffect(int8_t segmentID, int delta) {
        Segment& seg = strip.getSegment(segmentID);
        // You'll want to read the current effect and add the delta safely
        int newMode = constrain(seg.mode + delta, 0, 255); // Adjust upper bound based on actual WLED effect count
        seg.setMode(newMode);
        //colorUpdated(CALL_MODE_BUTTON); 
        stateUpdated(CALL_MODE_BUTTON);
        updateInterfaces(CALL_MODE_BUTTON);
    }

    void setSegmentEffectConfig(int8_t segmentID, int8_t configIndex, int8_t deltaValue) {
        Segment& seg = strip.getSegment(segmentID);
        int newValue;
        switch(configIndex) {
            case 0: newValue = seg.speed + deltaValue;     seg.speed = constrain(newValue, 0, 255); break;
            case 1: newValue = seg.intensity + deltaValue; seg.intensity = constrain(newValue, 0, 255); break;
            case 2: newValue = seg.opacity + deltaValue;   seg.opacity = constrain(newValue, 0, 255); break;
            case 3: newValue = seg.custom1 + deltaValue;   seg.custom1 = constrain(newValue, 0, 255); break;
            case 4: newValue = seg.custom2 + deltaValue;   seg.custom2 = constrain(newValue, 0, 255); break;
            case 5: newValue = seg.custom3 + deltaValue;   seg.custom3 = constrain(newValue, 0, 255); break;
            default: return;
        }
        stateUpdated(CALL_MODE_BUTTON);
        //colorUpdated(CALL_MODE_BUTTON);
    }

    // --- THE EVENT HANDLER ---
    void handleHardwareEvent(int encoderIndex, rotaryEncoder::ButtonEventType event, int16_t delta) {
        Rotary_Encoder_MODI& currentMode = encoderModes[encoderIndex];
        int8_t segID = encoderSegments[encoderIndex];

        // BUTTON LOGIC
        if (event == rotaryEncoder::ButtonEventType::SHORT_PRESS) {
            if (currentMode == Rotary_Encoder_MODI::TOGGLED_OFF) {
                currentMode = Rotary_Encoder_MODI::BRIGHTNESS_MODI;
                toggleSegment(segID);
                Serial.printf("Encoder %d: Segment %d ON (Brightness Mode)\n", encoderIndex, segID);
            }
            else if (currentMode == Rotary_Encoder_MODI::BRIGHTNESS_MODI) {
                currentMode = Rotary_Encoder_MODI::EFFECT_MODI;
                Serial.printf("Encoder %d: Switched to EFFECT\n", encoderIndex);
            }
            else if (currentMode == Rotary_Encoder_MODI::EFFECT_MODI) {
                currentMode = Rotary_Encoder_MODI::EFFECT_CONFIG_MODI;
                Serial.printf("Encoder %d: Switched to EFFECT CONFIG\n", encoderIndex);
            }
            else if (currentMode == Rotary_Encoder_MODI::EFFECT_CONFIG_MODI) {
                currentMode = Rotary_Encoder_MODI::BRIGHTNESS_MODI;
                Serial.printf("Encoder %d: Looped back to BRIGHTNESS\n", encoderIndex);
            }
        }
        else if (event == rotaryEncoder::ButtonEventType::LONG_PRESS) {
            if (currentMode != Rotary_Encoder_MODI::TOGGLED_OFF) {
                currentMode = Rotary_Encoder_MODI::TOGGLED_OFF;
                toggleSegment(segID);
                Serial.printf("Encoder %d: Segment %d OFF\n", encoderIndex, segID);
            }
        }

        // ROTATION LOGIC
        if (delta != 0) {
            if (currentMode == Rotary_Encoder_MODI::BRIGHTNESS_MODI) {
                setSegmentBrightness(segID, delta); 
                Serial.printf("Encoder %d: Brightness delta %d applied to seg %d\n", encoderIndex, delta, segID);
            }
            else if (currentMode == Rotary_Encoder_MODI::EFFECT_MODI) {
                setSegmentEffect(segID, delta);
            }
            else if (currentMode == Rotary_Encoder_MODI::EFFECT_CONFIG_MODI) {
                setSegmentEffectConfig(segID, static_cast<int8_t>(encoderConfigs[encoderIndex]), delta);
            }
        }

        int16_t absoluteValue = getCurrentSegmentValue(segID, currentMode, encoderConfigs[encoderIndex]);
        
        // Pass it to the dumb printer!
        oledDisplay.drawEncoderState(encoderIndex, segID, currentMode, encoderConfigs[encoderIndex], absoluteValue);
    }

    // Safely reads the current value from WLED based on what mode we are in
    int16_t getCurrentSegmentValue(int8_t segmentID, Rotary_Encoder_MODI mode, selectedEffectConfig config) {
        Segment& seg = strip.getSegment(segmentID);
        
        switch(mode) {
            case Rotary_Encoder_MODI::BRIGHTNESS_MODI: 
                return seg.opacity;
            case Rotary_Encoder_MODI::EFFECT_MODI: 
                return seg.mode;
            case Rotary_Encoder_MODI::EFFECT_CONFIG_MODI:
                switch(config) {
                    case selectedEffectConfig::SPEED: return seg.speed;
                    case selectedEffectConfig::INTENSITY: return seg.intensity;
                    case selectedEffectConfig::OPACITY: return seg.opacity;
                    case selectedEffectConfig::CUSTOM1: return seg.custom1;
                    case selectedEffectConfig::CUSTOM2: return seg.custom2;
                    case selectedEffectConfig::CUSTOM3: return seg.custom3;
                }
                break;
            default: 
                return 0;
        }
        return 0;
    }

public:
    void setup() override {
        Serial.println("Starting WLED Bridge Usermod...");

        for( int i = 0 ; i < 4; i++ ) {

            Segment& seg = strip.getSegment(encoderSegments[i]);
            if (seg.getOption(SEG_OPTION_ON)) {
                encoderModes[i] = Rotary_Encoder_MODI::BRIGHTNESS_MODI;
            } else {
                encoderModes[i] = Rotary_Encoder_MODI::TOGGLED_OFF; 
            }
            encoderConfigs[i] = selectedEffectConfig::SPEED;
            encoderSegments[i] = 0; 
        }

        hardwareManager.setEventHandler(
            [this](int idx, rotaryEncoder::ButtonEventType ev, int16_t d) { 
                this->handleHardwareEvent(idx, ev, d); 
            }
        );

        hardwareManager.enable(true);
        oledDisplay.setup(displayPinSDA, displayPinSCL);
        hardwareManager.setup();
    }

    void loop() override {
        hardwareManager.loop();
    }

    void addToConfig(JsonObject& root) override {
        JsonObject top = root.createNestedObject(FPSTR(_name));
        top[FPSTR(_enabled)] = enabled;
        top[FPSTR(_longShortPressThreshold)] = (int)longPressThreshold;
        top[FPSTR(_doublePressThreshold)] = (int)doublePressThreshold;
        top[FPSTR(_displayPinSDA)] = (int)displayPinSDA;
        top[FPSTR(_displayPinSCL)] = (int)displayPinSCL;

        top["Anzahl Encoder"] = NUM_ENCODERS;

        for (int i = 0; i < NUM_ENCODERS; i++) {
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top.createNestedObject(encoderName);

            // Read directly from the Bridge's local arrays
            encoderObj[FPSTR(_pinCLK)] = pinCLK[i];
            encoderObj[FPSTR(_pinDT)]  = pinDT[i];
            encoderObj[FPSTR(_pinSW)]  = pinSW[i];
            encoderObj[FPSTR(_segID)]  = encoderSegments[i];
        }
    }

    bool readFromConfig(JsonObject& root) override {
        JsonObject top = root[FPSTR(_name)];
        if (top.isNull()) return false;

        getJsonValue(top[FPSTR(_enabled)], enabled);
        getJsonValue(top[FPSTR(_longShortPressThreshold)], longPressThreshold);
        getJsonValue(top[FPSTR(_doublePressThreshold)], doublePressThreshold);
        getJsonValue(top[FPSTR(_displayPinSDA)], displayPinSDA);
        getJsonValue(top[FPSTR(_displayPinSCL)], displayPinSCL);

        int8_t newCount = NUM_ENCODERS;
        getJsonValue(top["Anzahl Encoder"], newCount);
        NUM_ENCODERS = max(1, min((int)newCount, MAX_ENCODERS));

        for (int i = 0; i < NUM_ENCODERS; i++) {
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top[encoderName];

            if (!encoderObj.isNull()) {
                // Save JSON data directly into the Bridge's local arrays
                getJsonValue(encoderObj[FPSTR(_pinCLK)], pinCLK[i]);
                getJsonValue(encoderObj[FPSTR(_pinDT)],  pinDT[i]);
                getJsonValue(encoderObj[FPSTR(_pinSW)],  pinSW[i]);
                getJsonValue(encoderObj[FPSTR(_segID)],  encoderSegments[i]);

                // Pass the newly read pins down to the hardware manager
                hardwareManager.setEncoderPins(i, pinCLK[i], pinDT[i], pinSW[i]);
            }
        }
        return true;
    }
};

extern WLED_Bridge Instance_wledBridge;