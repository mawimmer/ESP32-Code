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


// --- WLED PinManager Mock ---

// 1. Das Enum für die Besitzer nachbauen
namespace PinOwner {
    enum Type {
        None = 0,
        UM_Unspecified = 252
    };
}

// 2. Die PinManager Klasse simulieren
class PinManagerClass {
private:
    // Der ESP32 hat 40 mögliche GPIOs. Wir merken uns hier, ob einer belegt ist.
    bool allocatedPins[40] = {false}; 

public:
    // Mock für das Reservieren
    bool allocatePin(int pin, bool output, int owner) {
        if (pin < 0 || pin >= 40) return true; // -1 (deaktivierte Pins) einfach durchwinken
        
        if (allocatedPins[pin]) {
            Serial.printf("[Mock PinManager] FEHLER: Pin %d ist bereits blockiert!\n", pin);
            return false;
        }
        
        allocatedPins[pin] = true;
        Serial.printf("[Mock PinManager] SUCCESS: Pin %d erfolgreich reserviert.\n", pin);
        return true;
    }

    // Mock für das Freigeben
    void deallocatePin(int pin, int owner) {
        if (pin >= 0 && pin < 40) {
            allocatedPins[pin] = false;
            Serial.printf("[Mock PinManager] INFO: Pin %d wurde wieder freigegeben.\n", pin);
        }
    }

    // --- EIGENE HILFSFUNKTION FÜR DEINE TESTS ---
    // (Diese Funktion gibt es im echten WLED nicht. Du kannst sie in deiner 
    // main.cpp aufrufen, um künstlich einen Konflikt zu erzeugen!)
    void mockBlockPinForTest(int pin) {
        if (pin >= 0 && pin < 40) {
            allocatedPins[pin] = true;
            Serial.printf("[Mock Setup] Pin %d künstlich für Testzwecke blockiert.\n", pin);
        }
    }
};

// 3. WLED nutzt eine globale Instanz namens "pinManager"
extern PinManagerClass pinManager;

// Simuliert ein WLED Segment
struct Segment {
    uint8_t opacity = 255;
    uint8_t mode = 0;
    
    void setOption(uint8_t option, bool value) {
        Serial.printf("[Mock] Segment Option %d gesetzt auf %s\n", option, value ? "true" : "false");
    }
    void setMode(uint8_t newMode) {
        mode = newMode;
        Serial.printf("[Mock] Segment Mode gesetzt auf %d\n", mode);
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
    virtual void onStateChange(uint8_t mode) {}
};
