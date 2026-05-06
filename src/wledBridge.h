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
static const char _pulsesPerDent[] PROGMEM = "Pulses Per Dent";
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
    MenuLevel encoderMenuLevel[4];
    MenuCategory encoderMenuCategory[4];
    SettingsMenu encoderSettingsMenu[4];
    EffectSlider encoderEffectSlider[4];
    int8_t encoderSegments[4];

    // --- CONFIG VARIABLES ---
    bool enabled = true;
    int numEncoders = 1;
    int longPressThreshold = 500;
    int doublePressThreshold = 400;
    int displayPinSDA = 21; // Or whatever your default is
    int displayPinSCL = 22;

    const int MAXEFFECTID = strip.getModeCount() - 1;

    int8_t pinCLK[4] = {5, -1, -1, -1};
    int8_t pinDT[4]  = {6, -1, -1, -1};
    int8_t pinSW[4]  = {7, -1, -1, -1};
    int pulsesPerDent[4]  = {2, -1, -1, -1};

    // --- WLED HELPER FUNCTIONS (Absorbed from your old namespace) ---
    void toggleSegment(int8_t segmentID) {
        Segment& seg = strip.getSegment(segmentID);

        seg.setOption(SEG_OPTION_ON, !seg.getOption(SEG_OPTION_ON));
        stateUpdated(CALL_MODE_BUTTON);
    }

    void setSegmentBrightness(int8_t segmentID, int delta) {
        Segment& seg = strip.getSegment(segmentID);
        float norm = (float)seg.opacity / 255.0f;
        float factor = 0.1f + 1.5f * norm * norm;
        int32_t step = 1 + (int32_t)(factor * 20);
        
        int32_t change = delta * step;
        seg.opacity = constrain(seg.opacity + change, 1, 255);
        stateUpdated(CALL_MODE_BUTTON);
        
        static unsigned long lastUIUpdate = 0;
        if (millis() - lastUIUpdate > 250) {
            updateInterfaces(CALL_MODE_BUTTON);
            lastUIUpdate = millis();
        }
    }

    void setSegmentEffect(int8_t segmentID, int delta) {
        Segment& seg = strip.getSegment(segmentID);
        // You'll want to read the current effect and add the delta safely
        int newMode = constrain(seg.mode + delta, 0, MAXEFFECTID); // Adjust upper bound based on actual WLED effect count
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

    void cycleEffectConfig(int encoderIndex, int delta) {
        int current = static_cast<int>(encoderEffectSlider[encoderIndex]);
        int8_t segID = encoderSegments[encoderIndex];
        int effectID = strip.getSegment(segID).mode;

        // Try up to 6 times to find an active slider
        for (int attempts = 0; attempts < 6; attempts++) {
            current += (delta > 0) ? 1 : -1;

            // Wrap around (0 to 5)
            if (current > 5) current = 0;
            if (current < 0) current = 5;

            EffectSlider newConfig = static_cast<EffectSlider>(current);

            // Opacity is a segment setting, so it is ALWAYS valid.
            if (newConfig == EffectSlider::OPACITY) {
                encoderEffectSlider[encoderIndex] = newConfig;
                return;
            }

            // Map our enum to WLED's internal slider IDs
            uint8_t sliderIndex = 0;
            if (newConfig == EffectSlider::SPEED) sliderIndex = 0;
            else if (newConfig == EffectSlider::INTENSITY) sliderIndex = 1;
            else if (newConfig == EffectSlider::CUSTOM1) sliderIndex = 2;
            else if (newConfig == EffectSlider::CUSTOM2) sliderIndex = 3;
            else if (newConfig == EffectSlider::CUSTOM3) sliderIndex = 4;

            // Ask WLED what this slider is called for the current effect
            char sliderName[32] = {0};
            extractModeSlider(effectID, sliderIndex, sliderName, 31);

            // If WLED returns a real name (and not '!' which means unused), lock it in!
            if (sliderName[0] != '\0' && sliderName[0] != '!') {
                encoderEffectSlider[encoderIndex] = newConfig;
                return;
            }
        }
    }

    template <typename T>
    void cycleEnum(T& currentEnum, int delta) {
        int current = static_cast<int>(currentEnum);
        
        // Magically grab the total number of items in whatever Enum is passed in!
        int maxLimit = static_cast<int>(T::_COUNT) - 1; 
        
        current += (delta > 0) ? 1 : -1;
        
        if (current > maxLimit) current = 0;
        if (current < 0) current = maxLimit;
        
        currentEnum = static_cast<T>(current);
    }

    // --- THE EVENT HANDLER ---
    void handleHardwareEvent(int encoderIndex, rotaryEncoder::ButtonEventType event, int16_t delta) {
        MenuLevel& currentMenuLevel = encoderMenuLevel[encoderIndex];
        MenuCategory& currentMenuCategory = encoderMenuCategory[encoderIndex];
        int8_t segID = encoderSegments[encoderIndex];

        // BUTTON LOGIC

        //  SHORT PRESS

        if (event == rotaryEncoder::ButtonEventType::SHORT_PRESS) {

            switch(currentMenuLevel) {
                case MenuLevel::HOME :
                    currentMenuLevel = MenuLevel::MENU;
                    break;
                
                case MenuLevel::MENU :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                            currentMenuLevel = MenuLevel::SELECTING_L1;
                            break;

                        case MenuCategory::SETTINGS :
                            currentMenuLevel = MenuLevel::SELECTING_L1;
                            break;

                        case MenuCategory::NIGHTMODE :
                            //do something
                            break;

                        case MenuCategory::SUNRISE :
                            //do something
                            break;

                        default:
                            break;
                    }
                break;

                
                case MenuLevel::SELECTING_L1 :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                            currentMenuLevel = MenuLevel::EDITING;
                            break;

                        case MenuCategory::SETTINGS:
                            currentMenuLevel = MenuLevel::EDITING;
                            break;

                        default:
                            break;
                    }
                break;
            }

        }

        //  LONG PRESS
        else if (event == rotaryEncoder::ButtonEventType::LONG_PRESS) {

            switch(currentMenuLevel) {
                case MenuLevel::HOME :
                    toggleSegment(segID);
                    currentMenuLevel = MenuLevel::OFF;
                    break;

                case MenuLevel::EDITING :
                    switch(currentMenuCategory) {
                        case MenuCategory::SUNRISE :
                        case MenuCategory::NIGHTMODE :
                            currentMenuLevel = MenuLevel::MENU;
                            break;

                        case MenuCategory::SETTINGS :
                        case MenuCategory::EFFECTS :
                            currentMenuLevel = MenuLevel::SELECTING_L1;
                            break;
                        
                        default:
                            break;

                    }
                break;

                case MenuLevel::SELECTING_L1 :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                        case MenuCategory::SETTINGS :
                            currentMenuLevel = MenuLevel::MENU;
                            break;

                        default:
                            break;
                    }
                    break;

                case MenuLevel::MENU :
                    currentMenuLevel = MenuLevel::HOME;
                    break;



            }

        }

        //  DOUBLE PRESS
        else if (event == rotaryEncoder::ButtonEventType::DOUBLE_PRESS) {
            if(currentMenuLevel != MenuLevel::OFF) {
                currentMenuLevel = MenuLevel::HOME;
            }
        }

        // ROTATION LOGIC
        if (delta != 0) {

            switch(currentMenuLevel) {
                case MenuLevel::HOME :
                    setSegmentBrightness(segID, delta);
                    break;

                case MenuLevel::MENU :
                    cycleEnum(encoderMenuCategory[encoderIndex], delta);
                    break;

                case MenuLevel::SELECTING_L1 :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                            setSegmentEffect(segID, delta);
                            break;

                        case MenuCategory::SETTINGS :
                            cycleEnum(encoderSettingsMenu[encoderIndex], delta);
                            break;

                        default:
                            break;

                    }
                    break;

                case MenuLevel::EDITING :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                            setSegmentEffectConfig(segID, static_cast<int8_t>(encoderEffectSlider[encoderIndex]), delta);
                            break;

                        case MenuCategory::SUNRISE :
                            //do something
                            break;

                        case MenuCategory::SETTINGS :
                            //do something
                            break;

                        case MenuCategory::NIGHTMODE :
                            //do something
                            break;

                        default:
                            break;

                    }
                    break;

                default:
                    break;

            }
        }
    }
    
// --- DISPLAY UPDATE ---

    void updateDisplay(int encoderIndex) {
        MenuLevel level = encoderMenuLevel[encoderIndex];
        MenuCategory category = encoderMenuCategory[encoderIndex];

        switch(level) {
            case MenuLevel::HOME :
                //draw home/brightness
                break;

            case MenuLevel::MENU :
                //draw menu
                break;
            
            case MenuLevel::SELECTING_L1 :
                switch(category) {
                    case MenuCategory::EFFECTS :
                        //draw effects list
                        break;

                    case MenuCategory::SUNRISE :
                        //draw sunrise settings
                        break;
                    
                    case MenuCategory::NIGHTMODE :
                        //draw nightmode settings
                        break;

                    case MenuCategory::SETTINGS :
                        //draw settings selection menu
                        break;

                    default:
                        break;

                }
            break;

            case MenuLevel::SELECTING_L2 :
                switch(category) {
                    case MenuCategory::EFFECTS :
                        //draw effect sliders
                        break;

                    case MenuCategory::SUNRISE :
                        //not reachable yet
                        break;
                    
                    case MenuCategory::NIGHTMODE :
                        //not reachable yet
                        break;

                    case MenuCategory::SETTINGS :
                        //draw specific setting menu
                        break;

                    default:
                        break;

                }
            break;

            case MenuLevel::EDITING :
                switch(category) {
                    case MenuCategory::EFFECTS :
                        //draw effect slider value
                        break;

                    case MenuCategory::SUNRISE :
                        //not reachable yet
                        break;
                    
                    case MenuCategory::NIGHTMODE :
                        //not reachable yet
                        break;

                    case MenuCategory::SETTINGS :
                        //draw specific setting menu
                        break;

                    default:
                        break;

                }
                break;    

            default:
                break;

        }
    }

        // --- gather info for drawDisplay ---

        // OLD STUFF #####


        // int16_t absoluteValue = getCurrentSegmentValue(segID, currentMode, encoderConfigs[encoderIndex]);

        // char lineBuffer[64] = {0}; 
        
        // // 1. If looking at effects, grab the Effect Name
        // if (currentMode == Rotary_Encoder_MODI::EFFECT_MODI) {
        //     extractModeName(absoluteValue, JSON_mode_names, lineBuffer, 63);
        // }
        // // 2. If looking at configs, grab the specific Slider Name!
        // else if (currentMode == Rotary_Encoder_MODI::EFFECT_SELECT_SLIDERS_MODI || 
        //          currentMode == Rotary_Encoder_MODI::EFFECT_ADJUST_SLIDERS_MODI) {
                 
        //     if (encoderConfigs[encoderIndex] == selectedEffectConfig::OPACITY) {
        //         snprintf(lineBuffer, 63, "Opacity");
        //     } else {
        //         uint8_t sliderIndex = 0;
        //         if (encoderConfigs[encoderIndex] == selectedEffectConfig::SPEED) sliderIndex = 0;
        //         else if (encoderConfigs[encoderIndex] == selectedEffectConfig::INTENSITY) sliderIndex = 1;
        //         else if (encoderConfigs[encoderIndex] == selectedEffectConfig::CUSTOM1) sliderIndex = 2;
        //         else if (encoderConfigs[encoderIndex] == selectedEffectConfig::CUSTOM2) sliderIndex = 3;
        //         else if (encoderConfigs[encoderIndex] == selectedEffectConfig::CUSTOM3) sliderIndex = 4;
                
        //         // Fetch the custom name directly from WLED's internal memory
        //         extractModeSlider(strip.getSegment(segID).mode, sliderIndex, lineBuffer, 63);
                
        //         // Capitalize the first letter for UI aesthetics
        //         if(lineBuffer[0] >= 'a' && lineBuffer[0] <= 'z') lineBuffer[0] -= 32;
        //     }
        // }
        
        // Pass it to the dumb printer!
        //oledDisplay.drawEncoderState(encoderIndex, segID, currentMode, encoderConfigs[encoderIndex], absoluteValue, lineBuffer);
    // }

    // get strings and values to draw

    // int16_t getCurrentSegmentValue(int encoderIndex, int8_t segmentID, MenuLevel level, MenuCategory category, EffectSlider slider) {
    //     Segment& seg = strip.getSegment(segmentID);

    //     switch (level) {
    //         case MenuLevel::HOME :
    //             return seg.opacity;
    //             break;

    //         case MenuLevel::MENU :
    //             default:
    //                 break;

    //         case MenuLevel::SELECTING_L1 :
    //             switch (category) {
    //                 case MenuCategory::EFFECTS :   
    //                     extractModeName(absoluteValue, JSON_mode_names, lineBuffer, 63);
    //                     break;
    //                 case MenuCategory::NIGHTMODE :
    //                     //get some values
    //                     break;
                    
    //                 case MenuCategory::SUNRISE :
    //                     //get some values
    //                     break;

    //                 case MenuCategory::SETTINGS :
    //                     //get some values
    //                     break;
                
    //                 default:
    //                     break;
    //             }
    //         break;

    //         case MenuLevel::SELECTING_L2 :
    //             switch (category) {
    //                 case MenuCategory::EFFECTS :
    //                     getSliderName(encoderIndex, segmentID);
    //                     break;
                
    //                 default:
    //                     break;
    //             }
    //         break;

    //         case MenuLevel::EDITING :
    //             switch(category) {
    //                 case MenuCategory::EFFECTS :
    //                     switch(slider) {
    //                         case EffectSlider::SPEED: return seg.speed;
    //                         case EffectSlider::INTENSITY: return seg.intensity;
    //                         case EffectSlider::OPACITY: return seg.opacity;
    //                         case EffectSlider::CUSTOM1: return seg.custom1;
    //                         case EffectSlider::CUSTOM2: return seg.custom2;
    //                         case EffectSlider::CUSTOM3: return seg.custom3;
    //                     }
    //             }
    //             break;

        
    //         default:
    //             break;
    //     }

    //     return 0;
    // }
    void getSliderName(int encoderIndex, int8_t segmentID) {
                if (encoderEffectSlider[encoderIndex] == EffectSlider::OPACITY) {
            snprintf(lineBuffer, 63, "Opacity");
        } else {
            uint8_t sliderIndex = 0;
            if (encoderEffectSlider[encoderIndex] == EffectSlider::SPEED) sliderIndex = 0;
            else if (encoderEffectSlider[encoderIndex] == EffectSlider::INTENSITY) sliderIndex = 1;
            else if (encoderEffectSlider[encoderIndex] == EffectSlider::CUSTOM1) sliderIndex = 2;
            else if (encoderEffectSlider[encoderIndex] == EffectSlider::CUSTOM2) sliderIndex = 3;
            else if (encoderEffectSlider[encoderIndex] == EffectSlider::CUSTOM3) sliderIndex = 4;
            
            // Fetch the custom name directly from WLED's internal memory
            extractModeSlider(strip.getSegment(segmentID).mode, sliderIndex, lineBuffer, 63);
            
            // Capitalize the first letter for UI aesthetics
            if(lineBuffer[0] >= 'a' && lineBuffer[0] <= 'z') lineBuffer[0] -= 32;
        }
    }
        /**hand over state and values to know what to draw*/
    void drawDisplay(int encoderIndex, int8_t segmentID, MenuLevel level, MenuCategory category, int value, const char* lineBuffer = nullptr);


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
        top[FPSTR(_pulsesPerDent)] = (int)pulsesPerDent;
        

        top["Anzahl Encoder"] = NUM_ENCODERS;

        for (int i = 0; i < NUM_ENCODERS; i++) {
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top.createNestedObject(encoderName);

            // Read directly from the Bridge's local arrays
            encoderObj[FPSTR(_pinCLK)] = pinCLK[i];
            encoderObj[FPSTR(_pinDT)]  = pinDT[i];
            encoderObj[FPSTR(_pinSW)]  = pinSW[i];
            encoderObj[FPSTR(_pulsesPerDent)]  = pulsesPerDent[i];
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
                getJsonValue(encoderObj[FPSTR(_pulsesPerDent)],  pulsesPerDent[i]);
                getJsonValue(encoderObj[FPSTR(_segID)],  encoderSegments[i]);

                // Pass the newly read pins down to the hardware manager
                hardwareManager.setEncoderPins(i, pinCLK[i], pinDT[i], pinSW[i]);
            }
        }
        return true;
    }
};

extern WLED_Bridge Instance_wledBridge;