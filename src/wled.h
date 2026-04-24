#pragma once
#include <Arduino.h>

// Der Dummy für die WLED-Klasse
class Usermod {
  public:
    virtual void setup() {}
    virtual void loop() {}
};

#define REGISTER_USERMOD(x)

// In case I want to use wled variables

// extern byte bri;
// extern byte effectCurrent;
// #define CALL_MODE_DIRECT_CHANGE 0
// inline void stateUpdated(int mode) {};
