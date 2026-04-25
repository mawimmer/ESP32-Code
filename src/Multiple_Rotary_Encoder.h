#include "wled.h"
#include <Arduino.h>
#include <driver/pcnt.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//#define Serial Serial0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define MY_SDA_PIN 17
#define MY_SCL_PIN 18

/**
 * Rotary Encoder Specifications
 */
#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21
#define ROTARY_ENCODER_DETENTS 30


//Simulation variables
const int NUM_ENCODERS = 1;
bool standalone = false;



bool displayON = false;
volatile unsigned long lastUpdate = 0;

/**
 * Click Modi
 */
enum Rotary_Encoder_MODI {
    BRIGHTNESS_MODI,
    EFFECT_MODI,
    TOGGLED_OFF
};

/**
 * Button State (Umbenannt zur Vermeidung von Namenskonflikten)
 */
enum ButtonEventType {
    NONE,
    SHORT_PRESS,
    LONG_PRESS
};

/**
 * Struct Holding Information of each Encoder
 */
struct RotaryEncoder {

    /**
     * Hardware Info
     */
    pcnt_unit_t unit;
    gpio_num_t pin_clk;
    gpio_num_t pin_dt;
    gpio_num_t pin_sw;

    /**
     * Rotation Variables
     */
    int16_t deltaValue = 0;
    unsigned long TimeOfLastRotation = 0;
    bool rotationPending = false;
    int rotationDelay = 0;

    /**
     * Click Variables
     */
    volatile uint32_t TimeOfLastClick = 0;
    volatile uint32_t TimeOfButtonRelease = 0;
    volatile uint32_t lastEdge = 0;
    volatile bool buttonIsPressed = false;
    volatile bool buttonWasPressed = false;
    volatile bool buttonPressHandled = true;
    volatile bool longPressHandled = false;  // Track if long press was already fired
    volatile int lastPinLevel = HIGH;  // Track last known state to detect state CHANGES

    /**
     * Click and Rotation Execution States
     */
    ButtonEventType eventButton = NONE; // Hier den neuen Enum-Namen verwendet

    bool eventRotation = false;
    Rotary_Encoder_MODI MODI = TOGGLED_OFF;

    /**
     * Brightness and Effect Variables
     */
    int brightness = 0;
    int effect = 0;

    RotaryEncoder(pcnt_unit_t u, int clk, int dt, int sw) :
        unit(u), pin_clk(static_cast<gpio_num_t>(clk)), pin_dt(static_cast<gpio_num_t>(dt)), pin_sw(static_cast<gpio_num_t>(sw)) {};
};

/**
 * Configures and Sets Up a PCNT unit
 */
inline void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt) {
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = pin_clk,
        .ctrl_gpio_num = pin_dt,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .counter_h_lim = PCNT_LIMIT_HIGH,
        .counter_l_lim = PCNT_LIMIT_LOW,
        .unit = unit,
        .channel = PCNT_CHANNEL_0,
    };

    pcnt_unit_config(&pcnt_config);
    pcnt_set_filter_value(unit, 1000);
    pcnt_filter_enable(unit);
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);
    pcnt_counter_resume(unit);
}

/**
 * Hardware Interrupt Function
 */
void IRAM_ATTR buttonISR(void* arg) {
    RotaryEncoder& Encoder = *static_cast<RotaryEncoder*>(arg);
    
    uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    if (now - Encoder.lastEdge < 100) {
        return;  // Ignore bounces
    }
    
    Encoder.lastEdge = now;
    int pinLevel = gpio_get_level(Encoder.pin_sw);
    
    if (pinLevel == LOW && !Encoder.buttonIsPressed) {
        // Button PRESSED (first time, not a bounce)
        Encoder.TimeOfLastClick = now;  // Only set on initial press
        Encoder.buttonIsPressed = true;
        Encoder.buttonPressHandled = false;
        Encoder.longPressHandled = false;  // Reset long press flag
    } else if (pinLevel == HIGH && Encoder.buttonIsPressed) {
        // Button RELEASED
        Encoder.TimeOfButtonRelease = now;  // Capture release time immediately
        Encoder.buttonIsPressed = false;
        Encoder.buttonWasPressed = true;
    }
}


class Multiple_Rotary_Encoder : public Usermod {

private:
    unsigned long lastBlinkTime = 0;
    bool ledState = LOW;

    Adafruit_SSD1306 OLED_Display{SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET};

    void initOLED() {

        Wire.begin(MY_SDA_PIN, MY_SCL_PIN);

        if (!OLED_Display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("SSD1306 Fehler: Display nicht gefunden!"));
        };
        OLED_Display.clearDisplay();
        OLED_Display.setTextSize(1);
        OLED_Display.setTextColor(SSD1306_WHITE);
        OLED_Display.setCursor(0, 10);
        OLED_Display.println(F("Brightness:"));
        OLED_Display.setCursor(0, 20);
        OLED_Display.println(F("Effect:"));
        OLED_Display.display();
        Serial.println("Display initialized!");
    }


    int BRIGHTNESS_ROTATION_DELAY = 40;
    int EFFECT_ROTATION_DELAY = 150;
    const int LongShortPressThreshold = 500;
    bool global_eventPending = false;

    // Rotary Encoders Pin Declarations (actual ESP32-S3 Pinout Numbers)
    RotaryEncoder Encoders[NUM_ENCODERS]{
        {PCNT_UNIT_0, 5, 6, 7}
    };

    void init_PCNT_UNITS() {
        for (int i = 0; i < NUM_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];

            gpio_set_direction(Encoder.pin_clk, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_clk, GPIO_PULLUP_ONLY);
            gpio_set_direction(Encoder.pin_dt, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_dt, GPIO_PULLUP_ONLY);
            gpio_set_direction(Encoder.pin_sw, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_sw, GPIO_PULLUP_ONLY);

            gpio_set_intr_type(Encoder.pin_sw, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add(Encoder.pin_sw, buttonISR, &Encoder);

            setup_PCNT_UNIT(Encoder.unit, Encoder.pin_clk, Encoder.pin_dt);

            Serial.printf("Encoder # %d has been initialized! \r\n", i);
        }
    }

    void updateHardware() {
        for (int i = 0; i < NUM_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];

            // Check for long press while button is still held down
            if (Encoder.buttonIsPressed && !Encoder.longPressHandled) {
                uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
                uint32_t pressDuration = now - Encoder.TimeOfLastClick;
                
                if (pressDuration >= LongShortPressThreshold) {
                    Serial.printf("LONG PRESS DETECTED (duration: %lu ms)\n", pressDuration);
                    Encoder.eventButton = LONG_PRESS;
                    Encoder.longPressHandled = true;  // Prevent repeated triggers
                    global_eventPending = true;
                }
            }

            // Handle button release - for short presses
            if (!Encoder.buttonPressHandled) {
                if (Encoder.buttonWasPressed) {
                    // Only process as SHORT_PRESS if it wasn't already a LONG_PRESS
                    if (!Encoder.longPressHandled) {
                        uint32_t pressDuration = Encoder.TimeOfButtonRelease - Encoder.TimeOfLastClick;
                        
                        if (pressDuration < LongShortPressThreshold) {
                            Serial.printf("SHORT PRESS (duration: %lu ms)\n", pressDuration);
                            Encoder.eventButton = SHORT_PRESS;
                            global_eventPending = true;
                        }
                    }
                    
                    Encoder.buttonPressHandled = true;
                    Encoder.buttonWasPressed = false;
                }
            }

            // Rest stays the same...
            int16_t ValueNOW;
            pcnt_get_counter_value(Encoder.unit, &ValueNOW);

            
            if (ValueNOW) {
                pcnt_counter_clear(Encoder.unit);
                Encoder.deltaValue += ValueNOW;
                Encoder.rotationPending = true;
                Encoder.TimeOfLastRotation = xTaskGetTickCount() * portTICK_PERIOD_MS;
                Serial.printf("Encoder %d has a NEW Value: %d .Time of last Rotation: %d \r\n", i, ValueNOW, Encoder.TimeOfLastRotation);
            }

            if (Encoder.rotationPending && (xTaskGetTickCount() * portTICK_PERIOD_MS) - Encoder.TimeOfLastRotation >= Encoder.rotationDelay) {
                Encoder.rotationPending = false;
                Encoder.eventRotation = true;
                global_eventPending = true;
                Serial.printf("Encoder %d has a CONFIRMED Delta: %d \r\n", i, Encoder.deltaValue);
            }
        }
    }

    void global_EventHandler() {
        Serial.println("global_EventHandler");
        global_eventPending = false;

        for (int i = 0; i < NUM_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];

            if (Encoder.eventButton != NONE) {
                ButtonEventHandler(Encoder);
            }

            if (Encoder.eventRotation) {
                RotationEventHandler(Encoder);
            }
        }
    }

    void ButtonEventHandler(RotaryEncoder& Encoder) {
        Serial.println("ButtonEventHandler");
        if (Encoder.eventButton == SHORT_PRESS) {
            switch (Encoder.MODI) {
                case TOGGLED_OFF:
                    Serial.println("was TOGGLED OFF - now BRIGHTNESS");
                    OnOffDisplay(Encoder);
                    toggleOnOff();
                    stateUpdated(CALL_MODE_BUTTON);
                    updateInterfaces(CALL_MODE_BUTTON);
                    Encoder.MODI = BRIGHTNESS_MODI;
                    Encoder.rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                    pcnt_counter_resume(Encoder.unit);

                    break;
                case BRIGHTNESS_MODI:
                    Serial.println("was BRIGHTNESS - now EFFECT");
                    Encoder.MODI = EFFECT_MODI;
                    Encoder.rotationDelay = EFFECT_ROTATION_DELAY;
                    break;
                case EFFECT_MODI:
                    Serial.println("was EFFECT- now BRIGHTNESS");
                    Encoder.MODI = BRIGHTNESS_MODI;
                    Encoder.rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                    break;
            }
        } else if (Encoder.eventButton == LONG_PRESS) {
            Serial.println("LONG PRESS - Toggling OFF");
            toggleOnOff();
            stateUpdated(CALL_MODE_BUTTON);
            updateInterfaces(CALL_MODE_BUTTON);
            Encoder.MODI = TOGGLED_OFF;
            OnOffDisplay(Encoder);
            pcnt_counter_pause(Encoder.unit);
        }
        Encoder.eventButton = NONE;
    }

    void RotationEventHandler(RotaryEncoder& Encoder) {
        Serial.println("RotationEventHandler");
        Encoder.eventRotation = false;

        switch (Encoder.MODI) {
            case BRIGHTNESS_MODI:
                Brightness_Push(Encoder);
                updateDisplay(Encoder);
                break;
            case EFFECT_MODI:
                Encoder.effect += Encoder.deltaValue;
                updateDisplay(Encoder);
                break;
            case TOGGLED_OFF:
                break;
        }
        Encoder.deltaValue = 0;
    }


    void updateDisplay(RotaryEncoder& Encoder) {

        if( millis() - lastUpdate >= 50 ) {
            lastUpdate = millis();

            Serial.println("updateDisplay");

            OLED_Display.fillRect(90, 10, 50, 10, SSD1306_BLACK);
            OLED_Display.setCursor(90, 10);
            OLED_Display.println(Encoder.brightness);
            OLED_Display.fillRect(90, 20, 50, 10, SSD1306_BLACK);
            OLED_Display.setCursor(90, 20);
            OLED_Display.println(Encoder.effect);
            OLED_Display.display();
        };

    }

    void Brightness_Push(RotaryEncoder& Encoder) {
        Serial.println("Brightness_Push");
        float norm = (float)Encoder.brightness / 255.0f;
        float factor = 0.1f + 1.5f * norm * norm;
        int32_t step = 1 + (int32_t)(factor * 20);
        int32_t change = Encoder.deltaValue * step;
        int32_t newDuty = Encoder.brightness + change;

        if (newDuty < 1) newDuty = 1;
        if (newDuty > 255) newDuty = 255;

        Encoder.brightness = newDuty;

        //strip.setBrightness(Encoder.brightness, false);
        bri = Encoder.brightness;
        stateUpdated(CALL_MODE_BUTTON);
        updateInterfaces(CALL_MODE_BUTTON);
    }

    void OnOffDisplay(RotaryEncoder& Encoder) {
        if(displayON){
            Serial.println("Display OFF");
            displayON = false;
            OLED_Display.ssd1306_command(SSD1306_DISPLAYOFF);
            return;
        } else {
            Serial.println("Display ON");
            displayON = true;
            OLED_Display.ssd1306_command(SSD1306_DISPLAYON);
            return;
        }

    };

public:
    void setup() override {
        pinMode(LED_BUILTIN, OUTPUT);

    for (int i = 0; i < NUM_ENCODERS; i++) {
        Encoders[i].rotationDelay = BRIGHTNESS_ROTATION_DELAY;
    }
        gpio_install_isr_service(0);
        initOLED();
        init_PCNT_UNITS();
    }

    void loop() override {

        updateHardware();

        if (global_eventPending) {
            global_EventHandler();
        }
    }
};

static Multiple_Rotary_Encoder multiple_rotary_encoder;
REGISTER_USERMOD(multiple_rotary_encoder);
