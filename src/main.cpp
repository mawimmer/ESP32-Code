#include <Arduino.h>
#include <driver/pcnt.h>

//Serial0 for reading content in VSC Terminal -- needs to be removed for real hardware
#define Serial Serial0

#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21

// const int CLK = 5;
// const int DT = 6;
// const int SW = 7;

const int NUM_ENCODERS = 4;

unsigned long lastBlinkTime = 0;
bool ledState = LOW;


//Rotary Encoder Declarations



enum Rotary_Encoder_MODI {
    BRIGHTNESS_MODI,
    EFFECT_MODI
};

//Struct holding the information of each unit defined
struct RotaryEncoder{
    //Hardware Info
    pcnt_unit_t unit;
    int pin_clk;
    int pin_dt;
    int pin_sw;

    //Memory
    int16_t lastValue;
    //State Machine
    // unsigned long TimeOfLastClick;

    // unsigned long TimeOfLastRotation;
    // bool confirmed = false;
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

RotaryEncoder Encoders[NUM_ENCODERS]{
    {PCNT_UNIT_0, 5, 6, 7, 0},
    {PCNT_UNIT_1, 8, 9, 10, 0},
    {PCNT_UNIT_2, 17, 18, 21, 0},
    {PCNT_UNIT_3, 1, 2, 3, 0}

};


void setup() {
    Serial.begin(115200);
    Serial.println(String("ESP-IDF Version is: ") + esp_get_idf_version());
    pinMode(LED_BUILTIN, OUTPUT);
    // pinMode(CLK, INPUT_PULLUP);
    // pinMode(DT, INPUT_PULLUP);
    // pinMode(SW, INPUT_PULLUP);
    for (int i = 0 ; i < NUM_ENCODERS ; i++) {
        pinMode(Encoders[i].pin_clk, INPUT_PULLUP);
        pinMode(Encoders[i].pin_dt, INPUT_PULLUP);
        pinMode(Encoders[i].pin_sw, INPUT_PULLUP);

        setup_PCNT_UNIT(Encoders[i].unit, Encoders[i].pin_clk, Encoders[i].pin_dt );

        Serial.printf("Encoder # %d has been initialized!", i );
    };
};

void loop() {

    for (int i = 0 ; i < NUM_ENCODERS; i++) {

        int16_t ValueNOW;

        pcnt_get_counter_value(Encoders[i].unit, &ValueNOW);

        if(Encoders[i].lastValue != ValueNOW) {
            Serial.printf("Encoder %d has a new Value: %d", i, ValueNOW);
            Encoders[i].lastValue = ValueNOW;
        };

    };



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



};

