#include <encoderManager.h>
#include <rotaryEncoder.h>
//#include <wledBridge.h>

#if defined(WOKWI_SIM) || defined(MOCK_COMPILE)
#define Serial Serial0
#include <wled_mock.h>
#elif defined(WLED_DEV)
#include "../../WLED/wled00/wled.h"
#else
#include <wled.h>
#endif

/**
 * Hardware Interrupt Function
 */
void IRAM_ATTR buttonISR(void* arg) {
    rotaryEncoder& Encoder = *static_cast<rotaryEncoder*>(arg);

    uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;

    if (now - Encoder.lastEdge < 50) {
        return;  // Ignore bounces
    }

    Encoder.lastEdge = now;

    if (!Encoder.buttonIsPressed) {
        // Button PRESSED
        Encoder.timeOfLastClick = now;  // Only set on initial press
        Encoder.buttonIsPressed = true;
        Encoder.buttonWasPressed = false;
        Encoder.buttonPressHandled = false;
        return;
    }

    if (Encoder.buttonIsPressed) {
        // Button RELEASED
        Encoder.buttonIsPressed = false;
        Encoder.buttonWasPressed = true;
        return;
    }
}

void rotaryEncoder::updatePCNT_Unit(int i) {
    int16_t ValueNOW;
    pcnt_get_counter_value(unit, &ValueNOW);

    if (ValueNOW) {
        pcnt_counter_clear(unit);
        deltaValue += ValueNOW;
        rotationPending = true;
        timeOfLastRotation = xTaskGetTickCount() * portTICK_PERIOD_MS;
        Serial.printf("Encoder %d has a NEW Value: %d .Time of last Rotation: %d \r\n", i, ValueNOW,
                      timeOfLastRotation);
    }

    if (rotationPending && (xTaskGetTickCount() * portTICK_PERIOD_MS) - timeOfLastRotation >= rotationDelay) {
        rotationPending = false;
        eventRotation = true;
        stateChanged = true;
        //Instance_encoderManager.global_eventPending = true;
        Serial.printf("Encoder %d has a CONFIRMED Delta: %d \r\n", i, deltaValue);
    }
}

void rotaryEncoder::updateButtonState(int32_t timeNOW) {
    // If a falling Edge is detected from an Interrupt, the .buttonPressHandled Flag is set "false"
    if (!buttonPressHandled) {
        int32_t timeDifference = timeNOW - timeOfLastClick;

        // Serial.printf("timeNow: %d timeOfLastClick %d timeDifference: %d timeNow", timeNOW ,timeOfLastClick,
        // timeDifference ); Serial.printf("DEBUG: pressed=%d, wasPressed=%d, timeDiff=%ld, threshold=%d\n",
        // buttonIsPressed, buttonWasPressed, timeDifference, longShortPressThreshold);

        // Long Press Detection when Button is Held
        if (buttonIsPressed) {
            // If Pressed Longer or Equal to (const int longShortPressThreshold) -> Long Press
            if (timeDifference >= longShortPressThreshold) {
                Serial.println("LONG PRESS HOLD");
                eventButton = ButtonEventType::LONG_PRESS;
                stateChanged = true;
                //Instance_encoderManager.global_eventPending = true;

                // Reset .buttonPressHandled to "true", so no more execution until next button press
                buttonPressHandled = true;
            };
        };

        // Long and Short Press Detection when Button is Released
        if (buttonWasPressed) {
            // If Pressed Shorter than (const int longShortPressThreshold) -> Short Press
            if (timeDifference < longShortPressThreshold) {
                if (waitingForDoubleClick && (timeNOW - timeOfFirstClick <= doublePressThreshold)) {
                    // YEAH! DoubleClick!
                    eventButton = ButtonEventType::DOUBLE_PRESS;
                    waitingForDoubleClick = false;
                    buttonPressHandled = true;
                    Serial.println("double press!");
                    //Instance_encoderManager.global_eventPending = true;
                    return;
                }
                if (!waitingForDoubleClick) {
                    Serial.println("waiting for doubleclick");
                    waitingForDoubleClick = true;
                    timeOfFirstClick = timeNOW;
                    buttonPressHandled = true;
                }

                // If Pressed Longer or Equal to (const int longShortPressThreshold) -> Long Press
                // Actual Edge Case - When CPU takes longer than 500ms to check the Button Press
            } else {
                Serial.println("LONG PRESS RELEASE");
                eventButton = ButtonEventType::LONG_PRESS;
                //Instance_encoderManager.global_eventPending = true;
                stateChanged = true;

                //      Reset .buttonPressHandled to "true", so no more execution until next button press
                buttonPressHandled = true;
            };
        };
    };
    if (waitingForDoubleClick && (timeNOW - timeOfFirstClick > doublePressThreshold)) {
        Serial.println("SHORT PRESS");
        eventButton = ButtonEventType::SHORT_PRESS;
        //Instance_encoderManager.global_eventPending = true;
        stateChanged = true;

        // Reset .buttonPressHandled to "true", so no more execution until next button press
        buttonPressHandled = true;
        waitingForDoubleClick = false;
    }
}
