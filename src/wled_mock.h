#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

#if defined(WOKWI_SIM) || defined (MOCK_COMPILE)
    #define Serial Serial0
#elif defined(WLED_DEV)
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

#define CALL_MODE_BUTTON 1
#define SEG_OPTION_ON 0
#ifndef FPSTR
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif

#define REGISTER_USERMOD(name) 

namespace PinOwner {
    enum Type {
        None = 0,
        UM_Unspecified = 252
    };
}

class PinManagerClass {
private:
    bool allocatedPins[40] = {false}; 

public:
    bool allocatePin(int pin, bool output, int owner) {
        if (pin < 0 || pin >= 40) return true;
        
        if (allocatedPins[pin]) {
            Serial.printf("[Mock PinManager] FEHLER: Pin %d ist bereits blockiert!\r\n", pin);
            return false;
        }
        
        allocatedPins[pin] = true;
        Serial.printf("[Mock PinManager] SUCCESS: Pin %d erfolgreich reserviert.\r\n", pin);
        return true;
    }

    void deallocatePin(int pin, int owner) {
        if (pin >= 0 && pin < 40) {
            allocatedPins[pin] = false;
            Serial.printf("[Mock PinManager] INFO: Pin %d wurde wieder freigegeben.\r\n", pin);
        }
    }

    void mockBlockPinForTest(int pin) {
        if (pin >= 0 && pin < 40) {
            allocatedPins[pin] = true;
            Serial.printf("[Mock Setup] Pin %d künstlich für Testzwecke blockiert.\r\n", pin);
        }
    }
};

extern PinManagerClass pinManager;

struct Segment {
    uint8_t opacity = 255;
    uint8_t mode = 0;


    uint16_t speed = 128;
    uint16_t intensity = 128;
    uint8_t custom1 = 0;
    uint8_t custom2 = 0;
    uint8_t custom3 = 0;


    bool isOn = false; 
    
    void setOption(uint8_t option, bool value) {
            if (option == SEG_OPTION_ON) {
                isOn = value;
            }
            Serial.printf("[Mock] Segment Option %d gesetzt auf %s\r\n", option, value ? "true" : "false");
    }

    bool getOption(uint8_t option) {
        if (option == SEG_OPTION_ON) {
            return isOn;
        }
        return false;
    }

    void setMode(uint8_t newMode) {
        mode = newMode;
        Serial.printf("[Mock] Segment Mode gesetzt auf %d\r\n", mode);
    }
};

class Strip {
private:
    Segment dummySegment;
public:
    Segment& getSegment(int8_t id) {
        Serial.printf("[Mock] Hole Segment ID: %d\r\n", id);
        return dummySegment;
    }
    int getModeCount() {
        return 10;
    }
};

extern Strip strip;

inline void stateUpdated(uint8_t callMode) {
    Serial.printf("[Mock] stateUpdated(callMode=%d)\r\n", callMode);
}

inline void updateInterfaces(uint8_t callMode) {
    Serial.printf("[Mock] updateInterfaces(callMode=%d)\r\n", callMode);
}

template <typename T>
void getJsonValue(const JsonVariant& value, T& target) {
    if (!value.isNull()) {
        target = value.as<T>();
    }
}

class Usermod {
public:
    virtual void setup() {}
    virtual void loop() {}
    virtual void addToConfig(JsonObject& root) {}
    virtual bool readFromConfig(JsonObject& root) { return true; }
    virtual void onStateChange(uint8_t mode) {}
};


extern int JSON_mode_names;
inline void extractModeName(int effect,int JSON_mode_names ,char* effectName,int a) {
    Serial.printf("[Mock] JSON_mode_names\r\n");
}
inline void extractModeSlider(int effect,int JSON_mode_names ,char* effectName,int a) {
    Serial.printf("[Mock] JSON_mode_names\r\n");
}
