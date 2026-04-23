#include "wled.h"
//#include "hardware.h"
#include "include.h"


/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/wled-dev/WLED/wiki/Add-own-functionality
 * 
 * This is an example for a v2 usermod.
 * v2 usermods are class inheritance based and can (but don't have to) implement more functions, each of them is shown in this example.
 * Multiple v2 usermods can be added to one compilation easily.
 * 
 * Creating a usermod:
 * This file serves as an example. If you want to create a usermod, it is recommended to use usermod_v2_empty.h from the usermods folder as a template.
 * Please remember to rename the class and file to a descriptive name.
 * You may also use multiple .h and .cpp files.
 * 
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */

//class name. Use something descriptive and leave the ": public Usermod" part :)
class Multiple_Rotary_Encoder : public Usermod {

  private:

    unsigned long lastBlinkTime = 0;
    bool ledState = LOW;

    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

    void initDisplay () {
        Wire.begin();
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("SSD1306 Fehler: Display nicht gefunden!"));
        }
        // 1. clear display buffer
        display.clearDisplay();
        // 2. set fontsize
        display.setTextSize(1);
        // 3. set fontcolor
        display.setTextColor(SSD1306_WHITE);
        // 4. set starting point
        display.setCursor(0, 10);
        display.println(F("Brightness:"));
        display.setCursor(0, 20);
        display.println(F("Effect:"));
        // 6. transfer buffer to display
        display.display();
    };

    /**
     * Custom Behaviour Settings
     */

    int BRIGHTNESS_ROTATION_DELAY = 40;
    int EFFECT_ROTATION_DELAY = 150;


    //Long-ShortPress Setting; LongPress >= Treshold, ShortPress < Treshold
    const int LongShortPressThreshold = 500;

    /**
     * Global Events Flag
     */

    bool global_eventPending = false;

    /**
     * Rotary Encoders Pin Declarations (actual ESP32-S3 Pinout Numbers)
     */
    RotaryEncoder Encoders[NUM_ENCODERS]{
        {PCNT_UNIT_0, 5, 6, 7}//,
        // {PCNT_UNIT_1, 8, 9, 10},
        // {PCNT_UNIT_2, 17, 18, 21},
        // {PCNT_UNIT_3, 1, 2, 3}

    };

    void init_PCNT_UNITS () {

        for (int i = 0 ; i < NUM_ENCODERS ; i++) {

            // Referencing Encoder to Encoder[i] Pointer in the Setup Scope
            RotaryEncoder& Encoder = Encoders[i];

            gpio_set_direction(Encoder.pin_clk, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_clk, GPIO_PULLUP_ONLY);
            gpio_set_direction(Encoder.pin_dt, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_dt, GPIO_PULLUP_ONLY);
            gpio_set_direction(Encoder.pin_sw, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_sw, GPIO_PULLUP_ONLY);

            gpio_set_intr_type(Encoder.pin_sw, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add(Encoder.pin_sw, buttonISR, &Encoder);

            setup_PCNT_UNIT(Encoder.unit, Encoder.pin_clk, Encoder.pin_dt );

            Serial.printf("Encoder # %d has been initialized! \r\n", i );
        };

    };

    void updateHardware () {
    
        for ( int i = 0 ; i < NUM_ENCODERS; i++ ) {

            /**
             * Hardware - Click Detection
             */

            // Referencing Encoder to Encoder[i] Pointer in the for Scope
            RotaryEncoder& Encoder = Encoders[i];

            // If a falling Edge is detected from an Interrupt, the .buttonPressHandled Flag is set "false"
            if ( !Encoder.buttonPressHandled ) {

                // Long Press Detection when Button is Held
                if( Encoder.buttonIsPressed) {
                    unsigned long timeDifference = millis() - Encoder.TimeOfLastClick;

                    // If Pressed Longer or Equal to (const int LongShortPressThreshold) -> Long Press
                    if( timeDifference >= LongShortPressThreshold ) {
                        Serial.println("LONG PRESS HOLD");
                        Encoder.eventButton = LONG_PRESS;
                        global_eventPending = true;

                        // Reset .buttonPressHandled to "true", so no more execution until next button press
                        Encoder.buttonPressHandled = true;

                    };
                };

                // Long and Short Press Detection when Button is Released
                if( Encoder.buttonWasPressed) {
                    unsigned long timeDifference = millis() - Encoder.TimeOfLastClick;

                    // If Pressed Shorter than (const int LongShortPressThreshold) -> Short Press
                    if( timeDifference < LongShortPressThreshold ) {
                        Serial.println("SHORT PRESS");
                        Encoder.eventButton = SHORT_PRESS;
                        global_eventPending = true;

                        // Reset .buttonPressHandled to "true", so no more execution until next button press
                        Encoder.buttonPressHandled = true;

                    // If Pressed Longer or Equal to (const int LongShortPressThreshold) -> Long Press
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

            /**
             * Hardware - Rotation Detection
             */

            int16_t ValueNOW;

            // - if a value has changed
            pcnt_get_counter_value(Encoder.unit, &ValueNOW);
            if (ValueNOW) {
                pcnt_counter_clear(Encoder.unit);
                Encoder.deltaValue += ValueNOW;

                Encoder.rotationPending = true;
                Encoder.TimeOfLastRotation = millis();
                
                Serial.printf("Encoder %d has a NEW Value: %d \r\n", i, ValueNOW);

            };

            // - if a rotation value has changed and is pending to be forwared
            if (Encoder.rotationPending && millis() - Encoder.TimeOfLastRotation >= Encoder.rotationDelay) {
                Encoder.rotationPending = false;
                Encoder.eventRotation = true;
                global_eventPending = true;
                
                Serial.printf("Encoder %d has a CONFIRMED Delta: %d \r\n", i, Encoder.deltaValue );

            };

        };
    };

    void global_EventHandler() {
        Serial.println("global_EventHandler");
        global_eventPending = false;

        for( int i = 0 ; i < NUM_ENCODERS ; i++ ) {
            RotaryEncoder& Encoder = Encoders[i];

            if( Encoder.eventButton != NONE ){
                ButtonEventHandler(Encoder);
            };

            if( Encoder.eventRotation ) {
                RotationEventHandler(Encoder);
            };

        };
    };

    void ButtonEventHandler(RotaryEncoder& Encoder) {
        Serial.println("ButtonEventHandler");
        if( Encoder.eventButton == SHORT_PRESS ) {
            switch(Encoder.MODI) {

                case TOGGLED_OFF:
                    Encoder.MODI = BRIGHTNESS_MODI;
                    Encoder.rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                    pcnt_counter_resume(Encoder.unit);
                    updateDisplayBrightness(Encoder);
                    break;

                case BRIGHTNESS_MODI:
                    Encoder.MODI = EFFECT_MODI;
                    Encoder.rotationDelay = EFFECT_ROTATION_DELAY;
                    break;

                case EFFECT_MODI:
                    Encoder.MODI = BRIGHTNESS_MODI;
                    Encoder.rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                    break;
            };

        } else if( Encoder.eventButton == LONG_PRESS ) {
            Encoder.MODI = TOGGLED_OFF;
            pcnt_counter_pause(Encoder.unit);
        };

        Encoder.eventButton = NONE;
    };

    void RotationEventHandler(RotaryEncoder& Encoder) {
        Serial.println("RotationEventHandler");
        Encoder.eventRotation = false;

        switch(Encoder.MODI) {
            case BRIGHTNESS_MODI: {
                Brightness_Push(Encoder);
                updateDisplayBrightness(Encoder);
                break;
            }
            case EFFECT_MODI:
                Encoder.effect += Encoder.deltaValue;
                updateDisplayEffect(Encoder);
                break;   
        };
        Encoder.deltaValue = 0;
    };

    void updateDisplayBrightness (RotaryEncoder& Encoder) {
        Serial.println("updateDisplayBrightness");
        display.fillRect(90, 10, 50, 10, SSD1306_BLACK);
        display.setCursor(90, 10);
        display.println(Encoder.brightness);
        display.display();
    };

    void updateDisplayEffect (RotaryEncoder& Encoder) {
        Serial.println("updateDisplayEffect");
        display.setCursor(90, 20);
        display.fillRect(90, 20, 50, 10, SSD1306_BLACK);
        display.println(Encoder.effect);
        display.display();
    };

    void Brightness_Push ( RotaryEncoder& Encoder ) {

        Serial.println("Brightness_Push");

        float norm = (float)Encoder.brightness / 255.0f;

        float factor = 0.1f + 1.5f * norm * norm;

        int32_t step = 1 + (int32_t)(factor * 20);  // max ~30
        int32_t change = Encoder.deltaValue * step;

        int32_t newDuty = Encoder.brightness + change;

        if (newDuty < 1) newDuty = 1;
        if (newDuty > 255) newDuty = 255;

        Encoder.brightness = newDuty;
    };

  public:

    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)


    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() override {

        pinMode(LED_BUILTIN, OUTPUT);
        //main setup
        initDisplay();
        gpio_install_isr_service(0);
        init_PCNT_UNITS();
    }

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() override {
      // if usermod is disabled or called during strip updating just exit
      // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
      if (!enabled || strip.isUpdating()) return;

        /**
         * Hardware Detection
         */
        updateHardware();

        /**
         * Software - Input Processing
         */
        if(global_eventPending) {
            global_EventHandler();
        };


        //Code running check
        static bool initialized = false;

        if (millis() - lastBlinkTime >= 500) {

            lastBlinkTime = millis();
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState);

            if(!initialized){
                Serial.println("ESP32 is alive!");
                initialized = true;
            };
        
        };
    }


};



static Multiple_Rotary_Encoder multiple_rotary_encoder;
REGISTER_USERMOD(multiple_rotary_encoder);