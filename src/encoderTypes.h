#pragma once

enum class MenuLevel {
    OFF,
    HOME,
    MENU,
    SELECTING_L1,
    SELECTING_L2,
    EDITING,
    SLEEP,
    _COUNT
};
inline const char* LevelNames[] = {
    "OFF",
    "Brightness",
    "Menu",
    "SubMenu 1",
    "SubMenu 2",
    "Editing",
    "Sleep"
};


enum class MenuCategory {
    NIGHTMODE,
    SUNRISE,
    EFFECTS,
    SETTINGS,
    _COUNT
};
inline const char* CategoryNames[] = {
    "Nightmode",
    "Sunrise",
    "Effects",
    "Settings"
};


enum class SettingsMenu {
    DESIGN,
    ENCODER,
    _COUNT
};
inline const char* SettingsNames[] = {
    "Design",
    "Encoder"
};

enum class EffectSlider {
    SPEED,
    INTENSITY,
    OPACITY,
    CUSTOM1,
    CUSTOM2,
    CUSTOM3,
    _COUNT
};
