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
    float curveGamma[4] = {2.0, 2.0, 2.0, 2.0};
    int uiPercent[4] = {50, 50, 50, 50};
    bool needsUpdate[4] = {false, false, false, false};
    unsigned long lastInput = 0;

    // --- CONFIG VARIABLES ---
    bool enabled = true;
    int numEncoders = 1;
    int longPressThreshold = 750;
    int doublePressThreshold = 600;
    int displayPinSDA = 17; // Or whatever your default is
    int displayPinSCL = 18;

    const int MAXEFFECTID = strip.getModeCount() - 1;

    int8_t pinCLK[4] = {5, -1, -1, -1};
    int8_t pinDT[4]  = {6, -1, -1, -1};
    int8_t pinSW[4]  = {7, -1, -1, -1};
    int pulsesPerDent[4]  = {2, -1, -1, -1};

    // --- WLED HELPER FUNCTIONS (Absorbed from your old namespace) ---
    void toggleSegment(int8_t segmentID) {
        Segment& seg = strip.getSegment(segmentID);

        seg.setOption(SEG_OPTION_ON, !seg.getOption(SEG_OPTION_ON));
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
    }

    void syncFromWLED(int encoderIndex, int segId) {
        Segment& seg = strip.getSegment(segId);

        uiPercent[encoderIndex] = round(100.0f * sqrt(seg.opacity / 255.0f));
    }

    void onStateChange(uint8_t mode) override {
        Serial.printf("stateUpdated(callMode=%d)\r\n", mode);

        for (int i = 0; i < 4; i++) {
            syncFromWLED(i, 0);
            // needsUpdate[i] = true;
            // updateDisplay(i);
        }
    }

    void setCurve(int encoderIndex, int delta) {
        float currentGamma = curveGamma[encoderIndex];
        currentGamma += (delta / 20.0f);
        curveGamma[encoderIndex] = constrain(currentGamma, 1.0f, 3.0f);

    }

    void setSegmentBrightness(int encoderIndex, int8_t segmentID, int delta) {
        Segment& seg = strip.getSegment(segmentID);

        // UI remains simple linear %
        uiPercent[encoderIndex] += delta * 2;
        uiPercent[encoderIndex] = constrain(uiPercent[encoderIndex], 1, 100);

        float x = uiPercent[encoderIndex];
        float gamma = curveGamma[encoderIndex]; // e.g. 2.0–4.0

        float output;

        if (x <= 20.0f) {
            // perfectly linear low-end control
            output = 2.55f * x;
        } else {
            // curved upper range
            float t = (x - 20.0f) / 80.0f; // normalize 20→100 to 0→1

            output = 51.0f + 204.0f * pow(t, gamma);
        }

        seg.opacity = constrain((uint8_t)round(output), 1, 255);

        Serial.println(seg.opacity);
        Serial.println(uiPercent[encoderIndex]);
    }

    void setSegmentEffect(int8_t segmentID, int delta) {
        Segment& seg = strip.getSegment(segmentID);
        // You'll want to read the current effect and add the delta safely
        int newMode = constrain(seg.mode + delta, 0, MAXEFFECTID); // Adjust upper bound based on actual WLED effect count
        seg.setMode(newMode);
        //colorUpdated(CALL_MODE_BUTTON); 
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
        //updateInterfaces(CALL_MODE_BUTTON);
    }

    void setEffectSliderValue(int8_t segmentID, int8_t configIndex, int8_t deltaValue) {
        Segment& seg = strip.getSegment(segmentID);
        int newValue;
        int stepMultiplier = 3; 
        deltaValue *= stepMultiplier;
        switch(configIndex) {
            case 0: newValue = seg.speed + deltaValue;     seg.speed = constrain(newValue, 0, 255); break;
            case 1: newValue = seg.intensity + deltaValue; seg.intensity = constrain(newValue, 0, 255); break;
            case 2: newValue = seg.opacity + deltaValue;   seg.opacity = constrain(newValue, 0, 255); break;
            case 3: newValue = seg.custom1 + deltaValue;   seg.custom1 = constrain(newValue, 0, 255); break;
            case 4: newValue = seg.custom2 + deltaValue;   seg.custom2 = constrain(newValue, 0, 255); break;
            case 5: newValue = seg.custom3 + deltaValue;   seg.custom3 = constrain(newValue, 0, 255); break;
            default: return;
        }
        //stateUpdated(CALL_MODE_BUTTON);
        stateUpdated(CALL_MODE_DIRECT_CHANGE);
    }

    int getActiveSilderValue(int encoderIndex) {

        int8_t segID = encoderSegments[encoderIndex];
        Segment& seg = strip.getSegment(segID);
        EffectSlider activeSlider = encoderEffectSlider[encoderIndex];

        switch(activeSlider) {
            case EffectSlider::SPEED:     return seg.speed;
            case EffectSlider::INTENSITY: return seg.intensity;
            case EffectSlider::OPACITY:   return seg.opacity;
            case EffectSlider::CUSTOM1:   return seg.custom1;
            case EffectSlider::CUSTOM2:   return seg.custom2;
            case EffectSlider::CUSTOM3:   return seg.custom3;
            default: return 0;
        }

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
                case MenuLevel::OFF :
                    currentMenuLevel = MenuLevel::HOME;
                    toggleSegment(segID);
                    break;

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
                            currentMenuLevel = MenuLevel::SELECTING_L2;
                            break;

                        case MenuCategory::SETTINGS:
                            //currentMenuLevel = MenuLevel::SELECTING_L2;
                            currentMenuLevel = MenuLevel::EDITING;
                            break;

                        default:
                            break;
                    }
                break;

                case MenuLevel::SELECTING_L2 :
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
                    
                case MenuLevel::MENU :
                    currentMenuLevel = MenuLevel::HOME;
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

                case MenuLevel::SELECTING_L2 :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                        case MenuCategory::SETTINGS :
                            currentMenuLevel = MenuLevel::SELECTING_L1;
                            break;

                        default:
                            break;
                    }
                    break;

                case MenuLevel::EDITING :
                    switch(currentMenuCategory) {
                        case MenuCategory::SUNRISE :
                        case MenuCategory::NIGHTMODE :
                            currentMenuLevel = MenuLevel::MENU;
                            break;

                        case MenuCategory::SETTINGS :
                        case MenuCategory::EFFECTS :
                            //currentMenuLevel = MenuLevel::SELECTING_L2;
                            currentMenuLevel = MenuLevel::SELECTING_L2;
                            break;
                        
                        default:
                            break;

                    }
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
                    setSegmentBrightness(encoderIndex ,segID, delta);
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

                case MenuLevel::SELECTING_L2 :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                            cycleEffectConfig(encoderIndex, delta);
                            break;

                        case MenuCategory::SUNRISE :
                            //do something
                            break;

                        case MenuCategory::NIGHTMODE :
                            //do something
                            break;

                        case MenuCategory::SETTINGS :
                            //do something
                            break;

                        default:
                            break;
                    }
                    break;

                case MenuLevel::EDITING :
                    switch(currentMenuCategory) {
                        case MenuCategory::EFFECTS :
                            setEffectSliderValue(segID, static_cast<int8_t>(encoderEffectSlider[encoderIndex]), delta);
                            break;

                        case MenuCategory::SUNRISE :
                            //do something
                            break;

                        case MenuCategory::SETTINGS :
                            setCurve(encoderIndex, delta);
                            Serial.println(curveGamma[encoderIndex]);
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
        updateDisplay(encoderIndex);
        lastInput = xTaskGetTickCount() * portTICK_PERIOD_MS;
        oledDisplay.wakeUp(); // does not support multiple displays yet.

    }
    
// --- DISPLAY UPDATE ---

    void updateDisplay(int encoderIndex) {
        needsUpdate[encoderIndex] = false;
        int segID = encoderSegments[encoderIndex];
        Segment& segment = strip.getSegment(segID);
        MenuLevel level = encoderMenuLevel[encoderIndex];
        MenuCategory category = encoderMenuCategory[encoderIndex];

        switch(level) {
            case MenuLevel::HOME :{
                const char* sliderName = LevelNames[static_cast<int>(level)];
                oledDisplay.renderSliderScreen(sliderName, uiPercent[encoderIndex], 100, 0, 100, "OFF", "Menu", nullptr);
                //draw home/brightness
                break;
            }



            case MenuLevel::MENU : {
                const char* sliderName = CategoryNames[static_cast<int>(category)];
                oledDisplay.renderSelectionScreen(sliderName, "Home", "Select", nullptr);
                //draw menu
                break;
            }

            
            case MenuLevel::SELECTING_L1 :
                switch(category) {
                    case MenuCategory::EFFECTS : {
                        int currentMode = segment.mode;
                        extractModeName(currentMode, JSON_mode_names, lineBuffer, 63);
                        oledDisplay.renderSelectionScreen(lineBuffer, "Return", "Select", nullptr);
                        //draw effects list
                        break;
                    }


                    case MenuCategory::SUNRISE :
                        oledDisplay.renderSelectionScreen("SUNRISE_L1", "Return", "Select", nullptr);
                        //draw sunrise settings
                        break;
                    
                    case MenuCategory::NIGHTMODE :
                        oledDisplay.renderSelectionScreen("NIGHTMODE_L1", "Return", "Select", nullptr);
                        //draw nightmode settings
                        break;

                    case MenuCategory::SETTINGS : {
                        const char* name = SettingsNames[static_cast<int>(encoderSettingsMenu[encoderIndex])];
                        oledDisplay.renderSelectionScreen(name, "Return", "Select", nullptr);
                        //draw settings selection menu
                        break;         
                    }


                    default:
                        break;

                }
            break;

            case MenuLevel::SELECTING_L2 :
                switch(category) {
                    case MenuCategory::EFFECTS :
                        getSliderName(encoderIndex);
                        oledDisplay.renderSelectionScreen(lineBuffer, "Return", "Select", nullptr);
                        //draw effect sliders
                        break;

                    case MenuCategory::SUNRISE :
                        oledDisplay.renderSelectionScreen("SUNRISE_L2", "Return", "Select", nullptr);
                        //not reachable yet
                        break;
                    
                    case MenuCategory::NIGHTMODE :
                        oledDisplay.renderSelectionScreen("NIGHTMODE_L2", "Return", "Select", nullptr);
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
                        getSliderName(encoderIndex);
                        oledDisplay.renderSliderScreen(lineBuffer, getActiveSilderValue(encoderIndex), 255, 0, 100, "Return", nullptr, nullptr);
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
                        oledDisplay.renderSliderScreen("SETTINGS_L2", curveGamma[encoderIndex], 3, 1, 3,"Return", nullptr, nullptr);
                        break;

                    default:
                        break;

                }
                break;
                
            case MenuLevel::OFF :
                oledDisplay.renderStaticScreen("Segment OFF", "On", nullptr, nullptr);

            default:
                break;

        }
    }


    char lineBuffer[64] = {0};

    void getSliderName(int encoderIndex) {
            int8_t segmentID = encoderSegments[encoderIndex];
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

public:
    void setup() override {
        Serial.println("Starting WLED Bridge Usermod...");

        for( int i = 0 ; i < 4; i++ ) {

            Segment& seg = strip.getSegment(encoderSegments[i]);
            if (seg.getOption(SEG_OPTION_ON)) {
                encoderMenuLevel[i] = MenuLevel::HOME;
            } else {
                encoderMenuLevel[i] = MenuLevel::OFF; 
            }
            encoderEffectSlider[i] = EffectSlider::SPEED;
            encoderSegments[i] = 0; // delete that after testing!!
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

    void sleepTimer() {
        if(encoderMenuLevel[0] == MenuLevel::HOME) return;
        unsigned long timeNOW = xTaskGetTickCount() * portTICK_PERIOD_MS;
        int timeDifference = timeNOW - lastInput;
        if(timeDifference > 30000) {
            encoderMenuLevel[0] = MenuLevel::HOME;
            updateDisplay(0);
        } 
    }

    void loop() override {
        hardwareManager.loop();
        oledDisplay.loop();
        sleepTimer();
    }

};

extern WLED_Bridge Instance_wledBridge;