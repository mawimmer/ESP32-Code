#include <Arduino.h>
#include <driver/pcnt.h>
#include <encoderManager.h>
#include <rotaryEncoder.h>

#if defined(WOKWI_SIM)
    #define Serial Serial0
#endif

int NUM_ENCODERS = 1;
bool standalone = false;
bool displayON = false;
volatile unsigned long lastUpdate = 0;

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

void encoderManager::init_PCNT_UNITS() {
    for (int i = 0; i < NUM_ENCODERS; i++) {
        rotaryEncoder& Encoder = Encoders[i];

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

void encoderManager::updateHardware() {
    for (int i = 0; i < NUM_ENCODERS; i++) {
        // Referencing Encoder to Encoder[i] Pointer in the for Scope
        rotaryEncoder& Encoder = Encoders[i];
        int32_t timeNOW = xTaskGetTickCount() * portTICK_PERIOD_MS;

        Encoder.updateButtonState(timeNOW);

        Encoder.updatePCNT_Unit(i);
        
        if (Encoder.eventButton != rotaryEncoder::ButtonEventType::NONE || Encoder.eventRotation) {
            global_eventPending = true;
        }
    }
}

void encoderManager::global_EventHandler() {
    Serial.println("global_EventHandler");
    global_eventPending = false;

    for (int i = 0; i < NUM_ENCODERS; i++) {
        rotaryEncoder& Encoder = Encoders[i];

        if(!onEventTriggered) continue;

        if (Encoder.eventButton != rotaryEncoder::ButtonEventType::NONE) {
            onEventTriggered(i, Encoder.eventButton, 0);
            Encoder.eventButton = rotaryEncoder::ButtonEventType::NONE;
        }

        if (Encoder.eventRotation && onEventTriggered) {
            onEventTriggered(i, rotaryEncoder::ButtonEventType::NONE, Encoder.deltaValue);

            Encoder.eventRotation = false; 
            Encoder.deltaValue = 0;
        }
    }
}

void encoderManager::setup() {

    gpio_install_isr_service(0);

    init_PCNT_UNITS();
}

void encoderManager::setEncoderPins(int index, int clk, int dt, int sw) {
    if (index >= 0 && index < MAX_ENCODERS) {
        Encoders[index].pin_clk = static_cast<gpio_num_t>(clk);
        Encoders[index].pin_dt = static_cast<gpio_num_t>(dt);
        Encoders[index].pin_sw = static_cast<gpio_num_t>(sw);
    }
}

void encoderManager::setThresholds(int index, int longPress, int doublePress) {
    if (index >= 0 && index < MAX_ENCODERS) {
        // We will need to make these public in rotaryEncoder later if you want to tweak them, 
        // but for now, we just pass them through safely.
    }
}

void encoderManager::loop() {
    if (!enabled) {
        return;
    }

    updateHardware();

    if (global_eventPending) {
        global_EventHandler();
    }
}
