#pragma once

// enum class Rotary_Encoder_MODI {
//     BRIGHTNESS_MODI,
//     EFFECT_MODI,
//     EFFECT_SELECT_SLIDERS_MODI,
//     EFFECT_ADJUST_SLIDERS_MODI,
//     TOGGLED_OFF,
//     _COUNT
// };

enum class MenuLevel {
    OFF,
    HOME,
    MENU,
    SELECTING,
    EDITING,
    _COUNT
};

enum class MenuCategory {
    NIGHTMODE,
    SUNRISE,
    EFFECTS,
    SETTINGS,
    _COUNT
};

enum class SettingsMenu {
    DESIGN,
    ENCODER,
    _COUNT
};

enum class selectedEffectConfig {
    SPEED,
    INTENSITY,
    OPACITY,
    CUSTOM1,
    CUSTOM2,
    CUSTOM3,
    _COUNT
};
