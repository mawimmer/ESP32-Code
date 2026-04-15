#include <Arduino.h>
#include <driver/pcnt.h>

#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21

const int CLK = 5;
const int DT = 6;
const int SW = 7;

unsigned long lastBlinkTime = 0;
bool ledState = LOW;




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


void setup() {
//Serial0 for reading content in VSC Terminal
    Serial0.begin(115200);
    Serial0.println(String("ESP-IDF Version is: ") + esp_get_idf_version());
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(CLK, INPUT_PULLUP);
    pinMode(DT, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);
    setup_PCNT_UNIT(PCNT_UNIT_0, CLK, DT);
};

void loop() {
    static bool initialized = false;

    if (millis() - lastBlinkTime >= 500) {

        lastBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    
        if (ledState == HIGH) {

            int16_t countValue;
            int16_t* counterpointer = &countValue;

            if(!initialized){
                Serial0.println("ESP32 is alive!");
                pcnt_counter_clear(PCNT_UNIT_0);
                initialized = true;
            };

            pcnt_get_counter_value(PCNT_UNIT_0, counterpointer);
            Serial0.println(countValue);
        };
    };



};

