#pragma once

#include <Arduino.h>
#include <ArduinoJson.h> // Wichtig für JsonObject, JsonVariant etc.

// --- WLED Makros & Konstanten ---
#define CALL_MODE_BUTTON 1
#define SEG_OPTION_ON 0
#ifndef FPSTR
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif

// Makro abfangen, das WLED normalerweise nutzt, um den Usermod zu registrieren
#define REGISTER_USERMOD(name) 

// --- WLED Mock Klassen ---

// Simuliert ein WLED Segment
struct Segment {
    uint8_t opacity = 255;
    
    void setOption(uint8_t option, bool value) {
        Serial.printf("[Mock] Segment Option %d gesetzt auf %s\n", option, value ? "true" : "false");
    }
};

// Simuliert das WLED LED-Strip Objekt
class Strip {
private:
    Segment dummySegment;
public:
    Segment& getSegment(int8_t id) {
        Serial.printf("[Mock] Hole Segment ID: %d\n", id);
        return dummySegment;
    }
};

// --- Globale WLED Variablen und Funktionen ---
extern Strip strip; // Deklaration (Definition passiert in der main.cpp)

inline void stateUpdated(uint8_t callMode) {
    Serial.printf("[Mock] stateUpdated(callMode=%d)\n", callMode);
}

inline void updateInterfaces(uint8_t callMode) {
    Serial.printf("[Mock] updateInterfaces(callMode=%d)\n", callMode);
}

// Simuliert die JSON Helfer-Funktion von WLED
template <typename T>
void getJsonValue(const JsonVariant& value, T& target) {
    if (!value.isNull()) {
        target = value.as<T>();
    }
}

// Simuliert die WLED Usermod Basisklasse
class Usermod {
public:
    virtual void setup() {}
    virtual void loop() {}
    virtual void addToConfig(JsonObject& root) {}
    virtual bool readFromConfig(JsonObject& root) { return true; }
};
