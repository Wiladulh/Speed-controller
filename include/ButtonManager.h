#pragma once

#include <Arduino.h>
#include "Config.h"
#include "CommandManager.h"

class ButtonManager {
public:
    void begin(const AppConfig& config, CommandManager& commands);
    void service(uint32_t nowMs);

private:
    struct ButtonState {
        int pin = -1;
        bool stablePressed = false;
        bool lastRawPressed = false;
        uint32_t changedAt = 0;
        bool activeLow = true;
    };
    ButtonState buttons_[MAX_FANS][BUTTON_COUNT]{};
    uint8_t fanCount_ = 0;
    CommandManager* commands_ = nullptr;
};
