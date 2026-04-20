#include <Arduino.h>
#include <driver/pcnt.h>

//#### Simulation Parameters Start

//Serial0 for reading content in VSC Terminal -- needs to be removed for real hardware
#define Serial Serial0

/**
 * Number of Simulated Encoders
 */
const int NUM_ENCODERS = 4;

/**
 * Variables for Simulation Check
 */
unsigned long lastBlinkTime = 0;
bool ledState = LOW;

//#### Simulation Parameters End




/**
 * Rotary Encoder Specifications
 */

#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21
#define ROTARY_ENCODER_DETENTS 30

/**
 * Long-ShortPress Setting; LongPress >= Treshold, ShortPress < Treshold
 */
const int LongShortPressThreshold = 500;


/**
 * Click Modi
 */

enum Rotary_Encoder_MODI {
    BRIGHTNESS_MODI,
    EFFECT_MODI,
    TOGGLE_ON,
    TOGGLE_OFF
};


/**
 * Struct Holding Information of each Encoder
 */
struct RotaryEncoder{

    /**
     * Hardware Info
     * Unit#; GPIO-PIN# CLK; GPIO-PIN#-DT; GPIO-PIN#-SW
     */

    pcnt_unit_t unit;
    gpio_num_t pin_clk;
    gpio_num_t pin_dt;
    gpio_num_t pin_sw;

    /**
     * Rotation Variables
     */

    int16_t lastValue = 0;
    unsigned long TimeOfLastRotation = 0;
    bool rotationPending = false;

    /**
     * Click Variables
     */

    volatile unsigned long TimeOfLastClick = 0;
    volatile unsigned long lastEdge = 0;
    volatile bool buttonIsPressed = false;
    volatile bool buttonWasPressed = false;
    volatile bool buttonPressHandled = true;

    /**
     * Click and Rotation Execution States
     */

    enum Rotary_Encoder_MODI MODI = BRIGHTNESS_MODI;

    bool eventShortPressed = false;
    bool eventLongPressed = false;

    RotaryEncoder(pcnt_unit_t u, int clk, int dt, int sw):
    unit(u), pin_clk(static_cast<gpio_num_t>(clk)), pin_dt(static_cast<gpio_num_t>(dt)), pin_sw(static_cast<gpio_num_t>(sw)) {};
};


/**
 * Configures and Sets Up a PCNT unit
 * 
 * @param unit the unit which is initialized
 * @param pin_clk the GPIO port for pulse
 * @param pin_dt the GPIO port for ctrl
 * 
 * @return void
 */
void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt){

    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = pin_clk,
        .ctrl_gpio_num = pin_dt,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,
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

};


/**
 * Rotary Encoders Pin Declarations (actual ESP32-S3 Pinout Numbers)
 */
RotaryEncoder Encoders[NUM_ENCODERS]{
    {PCNT_UNIT_0, 5, 6, 7},
    {PCNT_UNIT_1, 8, 9, 10},
    {PCNT_UNIT_2, 17, 18, 21},
    {PCNT_UNIT_3, 1, 2, 3}

};


/**
 * Hardware Interrupt Function
 * This function is executed when a Rising or Falling Edge is detected
 */
void IRAM_ATTR buttonISR(void* arg) {
    // Referencing Encoder to Pointer in the ISR Scope
    RotaryEncoder& Encoder = *static_cast<RotaryEncoder*>(arg);

    if(millis() - Encoder.lastEdge >= 50) {

        if(gpio_get_level(Encoder.pin_sw) == LOW) {

            // prevents bouncing
            Encoder.lastEdge = millis();

            // gets time of last click to determine long vs short click
            Encoder.TimeOfLastClick = Encoder.lastEdge;

            // sets button is pressed until rising edge neglects it
            Encoder.buttonIsPressed = true;
            Encoder.buttonPressHandled = false;

            Encoder.buttonWasPressed = false;

        } else {
            // prevents bouncing        
            Encoder.lastEdge = millis();

            // shifts buttonIsPressed to buttonWasPressed
            Encoder.buttonIsPressed = false;
            Encoder.buttonWasPressed = true;

        };

    };

};

void setup() {
    Serial.begin(115200);
    Serial.printf("ESP-IDF Version is: %s\r\n", esp_get_idf_version());
    pinMode(LED_BUILTIN, OUTPUT);

    gpio_install_isr_service(0);

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

void loop() {


    // checking each Encoder -

    for ( int i = 0 ; i < NUM_ENCODERS; i++ ) {

        /**
         * Hardware - Click Detection
         */

        // Referencing Encoder to Encoder[i] Pointer in the Loop Scope
        RotaryEncoder& Encoder = Encoders[i];

        // If a falling Edge is detected from an Interrupt, the .buttonPressHandled Flag is set "false"
        if ( !Encoder.buttonPressHandled ) {

            // Long Press Detection when Button is Held
            if( Encoder.buttonIsPressed) {
                unsigned long timeDifference = millis() - Encoder.TimeOfLastClick;

                // If Pressed Longer or Equal to (const int LongShortPressThreshold) -> Long Press
                if( timeDifference >= LongShortPressThreshold ) {
                    Serial.println("LONG PRESS HOLD");

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

                    // Reset .buttonPressHandled to "true", so no more execution until next button press
                    Encoder.buttonPressHandled = true;

                // If Pressed Longer or Equal to (const int LongShortPressThreshold) -> Long Press
                // Actual Edge Case - When CPU takes longer than 500ms to check the Button Press
                } else {
                    Serial.println("LONG PRESS RELEASE");

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
        if (Encoder.lastValue != ValueNOW) {
            Encoder.rotationPending = true;
            Encoder.TimeOfLastRotation = millis();
            Serial.printf("Encoder %d has a NEW Value: %d \r\n", i, ValueNOW);
            Encoder.lastValue = ValueNOW;
        };

        // - if a rotation value has changed and is pending to be forwared
        if (Encoder.rotationPending && millis() - Encoder.TimeOfLastRotation >= 300) {
            Encoder.rotationPending = false;
            Serial.printf("Encoder %d has a CONFIRMED Value: %d \r\n", i, Encoder.lastValue );
        }

    };





    

            
        //     if(gpio_get_level(Encoder.pin_sw) == LOW && Encoder.lastButtonState == STATE_HIGH){
                
        //         if(millis() - Encoder.TimeOfLastClick >= 500){

        //             Encoder.lastButtonState = STATE_LOW;

        //             Encoder.TimeOfLastClick = millis();

        //             switch(Encoder.MODI) {

        //                 case BRIGHTNESS_MODI:
        //                     Encoder.MODI = EFFECT_MODI;
        //                     Serial.printf("Encoder %d switched to Modi: %d \r\n", i, Encoder.MODI);
        //                     break;
        //                 case EFFECT_MODI:
        //                     Encoder.MODI = BRIGHTNESS_MODI;
        //                     Serial.printf("Encoder %d switched to Modi: %d \r\n", i, Encoder.MODI);
        //                     break;
        //             };

        //         };
            
        //     };
        // };

        // if(gpio_get_level(Encoder.pin_sw) == HIGH && Encoder.lastButtonState == STATE_LOW){
        //     Encoder.lastButtonState = STATE_HIGH;
        // };


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


    //not sure if this is needed, will pause
    vTaskDelay(pdMS_TO_TICKS(10));

};



