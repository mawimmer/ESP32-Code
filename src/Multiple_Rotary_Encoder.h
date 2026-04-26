#include "wled.h"
#include <Arduino.h>
#include <driver/pcnt.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define Serial Serial0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define MY_SDA_PIN 21
#define MY_SCL_PIN 22

/**
 * Rotary Encoder Specifications
 */
#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21
#define ROTARY_ENCODER_DETENTS 30


const int MAX_ENCODERS = 4;
//int NUM_ENCODERS = 1;

//Simulation variables
bool standalone = false;



volatile unsigned long lastUpdate = 0;



//Config Variables

static const char _name[] PROGMEM = "Multiple Rotary Encoder";
static const char _enabled[] PROGMEM = "Enabled";
static const char _pinConflict[] PROGMEM = "Enabled";
static const char _active[] PROGMEM = "Encoder Activated";
static const char _pinCLK[] PROGMEM = "Pin CLK";
static const char _pinDT[] PROGMEM = "Pin DT";
static const char _pinSW[] PROGMEM = "Pin SW";
static const char _segID[] PROGMEM = "Segment ID";
static const char _longShortPressThreshold[] PROGMEM = "Long Press Threshold";
static const char _doublePressThreshold[] PROGMEM = "Double Press Threshold";

/**
 * Click Modi
 */
enum Rotary_Encoder_MODI {
    BRIGHTNESS_MODI,
    EFFECT_MODI,
    TOGGLED_OFF,
    EFFECT_CONFIG
};

/**
 * Button State (Umbenannt zur Vermeidung von Namenskonflikten)
 */
enum ButtonEventType {
    NONE,
    SHORT_PRESS,
    DOUBLE_PRESS,
    LONG_PRESS,
};

// enum ButtonActionSelected {
//     ACTION_NONE,
//     ACTION_TOGGLE_ON_OFF,
//     ACTION_BRIGHTNESS_EFFECT,
//     ACTION_EFFECT_CONFIG
// };

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

    bool active;
    bool pinConflict = false;

    /**
     * Rotation Hardware Variables
     */
    int16_t deltaValue = 0;
    unsigned long timeOfLastRotation = 0;
    bool rotationPending = false;
    int rotationDelay = 0;

    /**
     * Click Hardware Variables
     */
    volatile uint32_t timeOfLastClick = 0;
    volatile uint32_t lastEdge = 0;
    volatile bool buttonIsPressed = false;
    volatile bool buttonWasPressed = false;
    volatile bool buttonPressHandled = true;
    bool waitingForDoubleClick = false;
    u_int32_t timeOfFirstClick = 0;

    /**
     * Click and Rotation Execution States
     */
    ButtonEventType eventButton = NONE;
    // int actionShortPress = ACTION_BRIGHTNESS_EFFECT;
    // int actionDoublePress = ACTION_EFFECT_CONFIG;

    bool eventRotation = false;
    Rotary_Encoder_MODI MODI = TOGGLED_OFF;
    bool configSelected = false;

    /**
     * Brightness and Effect Variables
     */

     //is this needed or can i rely entirely on wled?
    int brightness = 0;
    int effect = 0;

    /**
     * Display
     */
    bool displayON = false;
    bool firstON = true;

    /**
     * Wled variables
     */

    int8_t segmentID;

    RotaryEncoder(pcnt_unit_t u, int clk, int dt, int sw, int8_t seg, bool active) :
        unit(u), pin_clk(static_cast<gpio_num_t>(clk)), pin_dt(static_cast<gpio_num_t>(dt)), pin_sw(static_cast<gpio_num_t>(sw)), segmentID(seg), active(active){};
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


class Multiple_Rotary_Encoder : public Usermod {

private:

    int longShortPressThreshold = 500;
    int doublePressThreshold = 200;

    bool enabled = false;

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
        
        Serial.println("Display initialized!");
    }


    int BRIGHTNESS_ROTATION_DELAY = 40;
    int EFFECT_ROTATION_DELAY = 150;
    bool global_eventPending = false;

    // Rotary Encoders Pin Declarations (actual ESP32-S3 Pinout Numbers)
    RotaryEncoder Encoders[MAX_ENCODERS]{
        {PCNT_UNIT_0, 5, 6, 7, 0, true},
        {PCNT_UNIT_1, -1, -1, -1, -1, false},
        {PCNT_UNIT_2, -1, -1, -1, -1, false},
        {PCNT_UNIT_3, -1, -1, -1, -1, false}
    };

    bool allocateEncoderPins(RotaryEncoder& Encoder) {
        // PinOwner::UM_Unspecified ist der WLED-Standard für "irgendein Usermod"
        // 'false' bedeutet: Das ist ein Input-Pin (kein Output wie LEDs)
        bool clk_ok = pinManager.allocatePin(Encoder.pin_clk, false, PinOwner::UM_Unspecified);
        bool dt_ok  = pinManager.allocatePin(Encoder.pin_dt, false, PinOwner::UM_Unspecified);
        bool sw_ok  = pinManager.allocatePin(Encoder.pin_sw, false, PinOwner::UM_Unspecified);

        // Wenn auch nur einer der Pins von etwas anderem belegt ist, brechen wir ab!
        if (!clk_ok || !dt_ok || !sw_ok) {
            Serial.printf("FEHLER: Pins für Encoder an CLK %d sind durch WLED blockiert!\n", Encoder.pin_clk);
            
            // Sauber aufräumen: Die Pins wieder freigeben, falls nur einer fehlschlug
            if (clk_ok) pinManager.deallocatePin(Encoder.pin_clk, PinOwner::UM_Unspecified);
            if (dt_ok)  pinManager.deallocatePin(Encoder.pin_dt, PinOwner::UM_Unspecified);
            if (sw_ok)  pinManager.deallocatePin(Encoder.pin_sw, PinOwner::UM_Unspecified);
            
            return false; // Registrierung fehlgeschlagen
        }
        return true; // Alles super, Pins gehören jetzt uns!
    }

    void deallocateEncoderPins(RotaryEncoder& Encoder) {
        if (Encoder.pin_clk >= 0) pinManager.deallocatePin(Encoder.pin_clk, PinOwner::UM_Unspecified);
        if (Encoder.pin_dt >= 0)  pinManager.deallocatePin(Encoder.pin_dt, PinOwner::UM_Unspecified);
        if (Encoder.pin_sw >= 0)  pinManager.deallocatePin(Encoder.pin_sw, PinOwner::UM_Unspecified);
    }

    void init_PCNT_UNITS() {
        for (int i = 0; i < MAX_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];

            if (!Encoder.active) {
                continue;
                Encoder.pinConflict = false;
            }

            if (!allocateEncoderPins(Encoder)) {
                // Wenn nicht, schalten wir diesen Encoder gnadenlos auf inaktiv
                // want to change to stay active if implemented with dynamic javascript page, but forward -1 for each pin.
                Encoder.active = false;
                Encoder.pinConflict = true;
                continue; 
            }

            Encoder.pinConflict = false;

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

    /**
     * Hardware - Click Detection
     */
    void updateButton(RotaryEncoder& Encoder, int32_t timeNOW) {
    // If a falling Edge is detected from an Interrupt, the .buttonPressHandled Flag is set "false"
        if ( !Encoder.buttonPressHandled ) {



            int32_t timeDifference = timeNOW - Encoder.timeOfLastClick;

            //Serial.printf("timeNow: %d timeOfLastClick %d timeDifference: %d timeNow", timeNOW ,Encoder.timeOfLastClick, timeDifference );
            //Serial.printf("DEBUG: pressed=%d, wasPressed=%d, timeDiff=%ld, threshold=%d\n", 
            //Encoder.buttonIsPressed, Encoder.buttonWasPressed, timeDifference, longShortPressThreshold);

            // Long Press Detection when Button is Held
            if( Encoder.buttonIsPressed) {
                

                // If Pressed Longer or Equal to (const int longShortPressThreshold) -> Long Press
                if( timeDifference >= longShortPressThreshold ) {
                    Serial.println("LONG PRESS HOLD");
                    Encoder.eventButton = LONG_PRESS;
                    global_eventPending = true;

                    // Reset .buttonPressHandled to "true", so no more execution until next button press
                    Encoder.buttonPressHandled = true;

                };
            };

            // Long and Short Press Detection when Button is Released
            if( Encoder.buttonWasPressed) {

                // If Pressed Shorter than (const int longShortPressThreshold) -> Short Press
                if( timeDifference < longShortPressThreshold ) {

                    if(Encoder.waitingForDoubleClick && (timeNOW - Encoder.timeOfFirstClick <= doublePressThreshold)) {
                        // YEAH! DoubleClick!
                        Encoder.eventButton = DOUBLE_PRESS;
                        Encoder.waitingForDoubleClick = false;
                        Encoder.buttonPressHandled = true;
                        Serial.println("double press!");
                        return;
                    } 
                    if(!Encoder.waitingForDoubleClick) {
                        Encoder.waitingForDoubleClick = true;
                        Encoder.timeOfFirstClick = timeNOW;
                        Encoder.buttonPressHandled = true;
                    }

                // If Pressed Longer or Equal to (const int longShortPressThreshold) -> Long Press
                // Actual Edge Case - When CPU takes longer than 500ms to check the Button Press
                } else {
                    Serial.println("LONG PRESS RELEASE");
                    Encoder.eventButton = LONG_PRESS;
                    global_eventPending = true;

                    //      Reset .buttonPressHandled to "true", so no more execution until next button press
                    Encoder.buttonPressHandled = true;
                };

            };
        };
        if(Encoder.waitingForDoubleClick && (timeNOW - Encoder.timeOfFirstClick > doublePressThreshold)) {
            Serial.println("SHORT PRESS");
            Encoder.eventButton = SHORT_PRESS;
            global_eventPending = true;

            // Reset .buttonPressHandled to "true", so no more execution until next button press
            Encoder.buttonPressHandled = true;
            Encoder.waitingForDoubleClick = false; 
        }

    }

    /**
     * Hardware - Rotation Detection
     */
    void updatePCNT_Unit(RotaryEncoder& Encoder, int i) {
        int16_t ValueNOW;
        pcnt_get_counter_value(Encoder.unit, &ValueNOW);

        if (ValueNOW) {
            pcnt_counter_clear(Encoder.unit);
            Encoder.deltaValue += ValueNOW;
            Encoder.rotationPending = true;
            Encoder.timeOfLastRotation = xTaskGetTickCount() * portTICK_PERIOD_MS;
            Serial.printf("Encoder %d has a NEW Value: %d .Time of last Rotation: %d \r\n", i, ValueNOW, Encoder.timeOfLastRotation);
        }

        if (Encoder.rotationPending && (xTaskGetTickCount() * portTICK_PERIOD_MS) - Encoder.timeOfLastRotation >= Encoder.rotationDelay) {
            Encoder.rotationPending = false;
            Encoder.eventRotation = true;
            global_eventPending = true;
            Serial.printf("Encoder %d has a CONFIRMED Delta: %d \r\n", i, Encoder.deltaValue);
        }
    }

    void updateHardware() {

        for (int i = 0; i < MAX_ENCODERS; i++) {

            // Referencing Encoder to Encoder[i] Pointer in the for Scope
            RotaryEncoder& Encoder = Encoders[i];
            int32_t timeNOW = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            if(!Encoder.active) continue;

            updateButton(Encoder, timeNOW);

            updatePCNT_Unit(Encoder, i);
        }
    }

    void global_EventHandler() {
        Serial.println("global_EventHandler");
        global_eventPending = false;

        for (int i = 0; i < MAX_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];

            if(!Encoder.active) continue;

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
                case TOGGLED_OFF: {
                    Serial.println("was TOGGLED OFF - now BRIGHTNESS");
                    OnOffDisplay(Encoder);

                    Segment& seg = strip.getSegment(Encoder.segmentID);
                    seg.setOption(SEG_OPTION_ON, true);
                    stateUpdated(CALL_MODE_BUTTON);
                    updateInterfaces(CALL_MODE_BUTTON);

                    Encoder.MODI = BRIGHTNESS_MODI;
                    Encoder.rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                    pcnt_counter_resume(Encoder.unit);

                    break;
                }
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
        } else if (Encoder.eventButton == LONG_PRESS && Encoder.MODI != TOGGLED_OFF) {
            Serial.println("LONG PRESS - Toggling OFF");
            Segment& seg = strip.getSegment(Encoder.segmentID);
            seg.setOption(SEG_OPTION_ON, false);
            stateUpdated(CALL_MODE_BUTTON);
            updateInterfaces(CALL_MODE_BUTTON);
            Encoder.MODI = TOGGLED_OFF;
            OnOffDisplay(Encoder);
            pcnt_counter_pause(Encoder.unit);
        } else if (Encoder.eventButton == DOUBLE_PRESS && (Encoder.MODI == EFFECT_MODI || Encoder.MODI == EFFECT_CONFIG)) {
            if(!Encoder.configSelected) {
                Serial.println("In Config");
                Encoder.configSelected = true;
                Encoder.MODI = EFFECT_CONFIG;
            } else {
                Serial.println("Config leaving...");
                Encoder.configSelected = false;
                Encoder.MODI = EFFECT_MODI;
            }
        }
        Encoder.eventButton = NONE;
    }

    void RotationEventHandler(RotaryEncoder& Encoder) {
        Serial.println("RotationEventHandler");
        Encoder.eventRotation = false;

        switch (Encoder.MODI) {
            case BRIGHTNESS_MODI:
                brightnessPush(Encoder);
                updateDisplay(Encoder);
                break;
            case EFFECT_MODI:
                Encoder.effect += Encoder.deltaValue;
                changeEffect(Encoder);
                updateDisplay(Encoder);
                break;
            case TOGGLED_OFF:
                break;
        }
        Encoder.deltaValue = 0;
    }


    void updateDisplay(RotaryEncoder& Encoder) {

        if( millis() - lastUpdate >= 50 ) { // change 50 with a define placeholder or const, or even as config...
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

    void brightnessPush(RotaryEncoder& Encoder) {
        Serial.println("brightnessPush");
        float norm = (float)Encoder.brightness / 255.0f;
        float factor = 0.1f + 1.5f * norm * norm;   // make the rotation factor configurable. maybe decision between 
        int32_t step = 1 + (int32_t)(factor * 20);  // linear; steps; exponential with pow plus multfactor changeable?
        int32_t change = Encoder.deltaValue * step;
        int32_t newDuty = Encoder.brightness + change;

        if (newDuty < 1) newDuty = 1;
        if (newDuty > 255) newDuty = 255;

        Encoder.brightness = newDuty;

        //strip.setBrightness(Encoder.brightness, false);
        Segment& seg = strip.getSegment(Encoder.segmentID);
        seg.opacity = Encoder.brightness;
        stateUpdated(CALL_MODE_BUTTON);
        updateInterfaces(CALL_MODE_BUTTON);
    }

    void changeEffect(RotaryEncoder& Encoder) {
        Segment& seg = strip.getSegment(Encoder.segmentID);
        seg.setMode(Encoder.effect);
        stateUpdated(CALL_MODE_BUTTON);
        updateInterfaces(CALL_MODE_BUTTON);
    }

    void OnOffDisplay(RotaryEncoder& Encoder) {
        if(Encoder.displayON){
            Serial.println("Display OFF");
            Encoder.displayON = false;
            OLED_Display.ssd1306_command(SSD1306_DISPLAYOFF);
            return;
        } else {
            if(Encoder.firstON) {
                updateDisplay(Encoder);
                Encoder.firstON = false;
            }
            Serial.println("Display ON");
            Encoder.displayON = true;
            OLED_Display.ssd1306_command(SSD1306_DISPLAYON);
            return;
        }

    };

public:

    inline void enable(bool enable) { enabled = enable; }

    inline bool isEnabled() { return enabled; }

    void onStateChange(uint8_t callMode) override {

        //dont update if we forced the update with encoder input
        if (callMode == CALL_MODE_BUTTON) return;

        bool displayNeedsUpdate = false;

        for (int i = 0; i < MAX_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];
            Segment& seg = strip.getSegment(Encoder.segmentID);

            if(!Encoder.active) continue;

            // Wir vergleichen unsere internen Werte mit den tatsächlichen WLED-Werten
            if (Encoder.brightness != seg.opacity || Encoder.effect != seg.mode) {
                
                // Neue Werte aus WLED übernehmen
                Encoder.brightness = seg.opacity;
                Encoder.effect = seg.mode;
                
                displayNeedsUpdate = true;
            }
        }

        // 3. Wenn das Webinterface etwas geändert hat, Display neu zeichnen
        if (displayNeedsUpdate) {
            // HINWEIS: Wenn du wirklich mehrere Encoder nutzt, solltest du hier 
            // eine allgemeine "refreshAllDisplays()" Funktion aufrufen.
            // Fürs Erste (mit einem Encoder) reicht das hier:
            updateDisplay(Encoders[0]);
        }
    }

    void setup() override {

    for (int i = 0; i < MAX_ENCODERS; i++) {
        Encoders[i].rotationDelay = BRIGHTNESS_ROTATION_DELAY;
    }
        gpio_install_isr_service(0);
        initOLED();
        init_PCNT_UNITS();
    }

    void addToConfig(JsonObject& root) override {
        JsonObject top = root.createNestedObject(FPSTR(_name));
        top[FPSTR(_enabled)] = (bool)enabled;
        top[FPSTR(_longShortPressThreshold)]  = (int)longShortPressThreshold;
        top[FPSTR(_doublePressThreshold)]  = (int)doublePressThreshold;      

        //top["Anzahl Encoder"] = NUM_ENCODERS;

        

        for (int i = 0; i < MAX_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top.createNestedObject(encoderName);

            // Wichtig: (int8_t) erzwingt eine saubere Zahl im Webinterface
            encoderObj[FPSTR(_active)] = (bool)Encoder.active;

            if (Encoder.active && Encoder.pinConflict) {
                encoderObj["ERROR"] = F("⚠️ Pins already used! Check Pin-Info.");
            }

            top[FPSTR(_pinConflict)] = (bool)Encoder.pinConflict;
            encoderObj[FPSTR(_pinCLK)] = (int8_t)Encoder.pin_clk;
            encoderObj[FPSTR(_pinDT)]  = (int8_t)Encoder.pin_dt;
            encoderObj[FPSTR(_pinSW)]  = (int8_t)Encoder.pin_sw;
            
            encoderObj[FPSTR(_segID)]  = (int8_t)Encoder.segmentID;
        }
    }

    bool readFromConfig(JsonObject& root) override {
        JsonObject top = root[FPSTR(_name)];
        if (top.isNull()) return false;

        getJsonValue(top[FPSTR(_enabled)], enabled);
        getJsonValue(top[FPSTR(_longShortPressThreshold)], longShortPressThreshold);
        getJsonValue(top[FPSTR(_doublePressThreshold)], doublePressThreshold);

       // int8_t newCount = NUM_ENCODERS;
       // getJsonValue(top["Anzahl Encoder"], newCount);
       // NUM_ENCODERS = max(1, min((int)newCount, MAX_ENCODERS));

        for (int i = 0; i < MAX_ENCODERS; i++) {
            RotaryEncoder& Encoder = Encoders[i];
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top[encoderName];

            if (!encoderObj.isNull()) {
                bool active = Encoder.active;
                int8_t pCLK = Encoder.pin_clk;
                int8_t pDT  = Encoder.pin_dt;
                int8_t pSW  = Encoder.pin_sw;
                int8_t sID  = Encoder.segmentID;

                getJsonValue(encoderObj[FPSTR(_active)], active);
                getJsonValue(encoderObj[FPSTR(_pinCLK)], pCLK);
                getJsonValue(encoderObj[FPSTR(_pinDT)], pDT);
                getJsonValue(encoderObj[FPSTR(_pinSW)], pSW);
                getJsonValue(encoderObj[FPSTR(_segID)], sID); // <-- NEU: Aus Web-UI lesen

                Encoder.active = active;
                Encoder.pin_clk = static_cast<gpio_num_t>(pCLK);
                Encoder.pin_dt  = static_cast<gpio_num_t>(pDT);
                Encoder.pin_sw  = static_cast<gpio_num_t>(pSW);
                Encoder.segmentID  = sID; // <-- NEU: Neuen Wert speichern
            }
        }
        return true;
    }

    void loop() override {

        if(!enabled){
            return;
        }

        updateHardware();

        if (global_eventPending) {
            global_EventHandler();
        }
    }
};

static Multiple_Rotary_Encoder multiple_rotary_encoder;
REGISTER_USERMOD(multiple_rotary_encoder);
