#include <Arduino.h>
#include <driver/pcnt.h>

//Serial0 for reading content in VSC Terminal -- needs to be removed for real hardware
#define Serial Serial0

#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21

#define ROTARY_ENCODER_DETENTS 30


const int NUM_ENCODERS = 4;

unsigned long lastBlinkTime = 0;
bool ledState = LOW;


//Rotary Encoder Declarations



enum Rotary_Encoder_MODI {
    BRIGHTNESS_MODI,
    EFFECT_MODI
};

enum STATE_HIGH_LOW {
    STATE_HIGH,
    STATE_LOW
};

//Struct holding the information of each unit defined
struct RotaryEncoder{
    //Hardware Info
    pcnt_unit_t unit;
    gpio_num_t pin_clk;
    gpio_num_t pin_dt;
    gpio_num_t pin_sw;

    //Memory
    int16_t lastValue = 0;

    //State Machine

    unsigned long TimeOfLastClick = 0;
    unsigned long TimeOfLastRotation = 0;
    bool rotationPending = false;
    volatile bool buttonPressedFlag = false;

    enum Rotary_Encoder_MODI MODI = BRIGHTNESS_MODI;
    enum STATE_HIGH_LOW lastButtonState = STATE_HIGH;

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

RotaryEncoder Encoders[NUM_ENCODERS]{
    {PCNT_UNIT_0, 5, 6, 7},
    {PCNT_UNIT_1, 8, 9, 10},
    {PCNT_UNIT_2, 17, 18, 21},
    {PCNT_UNIT_3, 1, 2, 3}

};


void IRAM_ATTR buttonISR(void* arg) {
    RotaryEncoder* encoder = static_cast<RotaryEncoder*>(arg);
    encoder->buttonPressedFlag = true; 

};

void setup() {
    Serial.begin(115200);
    Serial.printf("ESP-IDF Version is: %s\r\n", esp_get_idf_version());
    pinMode(LED_BUILTIN, OUTPUT);

    for (int i = 0 ; i < NUM_ENCODERS ; i++) {

        gpio_set_direction(Encoders[i].pin_clk, GPIO_MODE_INPUT);
        gpio_set_pull_mode(Encoders[i].pin_clk, GPIO_PULLUP_ONLY);
        gpio_set_direction(Encoders[i].pin_dt, GPIO_MODE_INPUT);
        gpio_set_pull_mode(Encoders[i].pin_dt, GPIO_PULLUP_ONLY);
        gpio_set_direction(Encoders[i].pin_sw, GPIO_MODE_INPUT);
        gpio_set_pull_mode(Encoders[i].pin_sw, GPIO_PULLUP_ONLY);

        setup_PCNT_UNIT(Encoders[i].unit, Encoders[i].pin_clk, Encoders[i].pin_dt );

        Serial.printf("Encoder # %d has been initialized! \r\n", i );
    };


};

void loop() {


    //checking each Encoder -

    for (int i = 0 ; i < NUM_ENCODERS; i++) {

        //if (Encoders[i].buttonPressedFlag) {
        
            //Encoders[i].buttonPressedFlag = false;

            
            if(gpio_get_level(Encoders[i].pin_sw) == LOW && Encoders[i].lastButtonState == STATE_HIGH){
                
                if(millis() - Encoders[i].TimeOfLastClick >= 500){

                    Encoders[i].lastButtonState = STATE_LOW;

                    Encoders[i].TimeOfLastClick = millis();

                    switch(Encoders[i].MODI) {

                        case BRIGHTNESS_MODI:
                            Encoders[i].MODI = EFFECT_MODI;
                            Serial.printf("Encoder %d switched to Modi: %d \r\n", i, Encoders[i].MODI);
                            break;
                        case EFFECT_MODI:
                            Encoders[i].MODI = BRIGHTNESS_MODI;
                            Serial.printf("Encoder %d switched to Modi: %d \r\n", i, Encoders[i].MODI);
                            break;
                    };

                };
            
            };
        //};

        if(gpio_get_level(Encoders[i].pin_sw) == HIGH && Encoders[i].lastButtonState == STATE_LOW){
            Encoders[i].lastButtonState = STATE_HIGH;
        };
       

        int16_t ValueNOW;

        // - if a value has changed
        pcnt_get_counter_value(Encoders[i].unit, &ValueNOW);
        if (Encoders[i].lastValue != ValueNOW) {
            Encoders[i].rotationPending = true;
            Encoders[i].TimeOfLastRotation = millis();
            Serial.printf("Encoder %d has a NEW Value: %d \r\n", i, ValueNOW);
            Encoders[i].lastValue = ValueNOW;
        };

        // - if a rotation value has changed and is pending to be forwared
        if (Encoders[i].rotationPending && millis() - Encoders[i].TimeOfLastRotation >= 300) {
            Encoders[i].rotationPending = false;
            Serial.printf("Encoder %d has a CONFIRMED Value: %d \r\n", i, Encoders[i].lastValue );
        }

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

    //not sure if this is needed, will pause
    vTaskDelay(pdMS_TO_TICKS(10));

};

