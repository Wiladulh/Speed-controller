#include "ButtonManager.h"

void ButtonManager::begin(const AppConfig& config, CommandManager& commands) {
    commands_ = &commands;
    fanCount_ = config.fanCount;
    for (uint8_t f = 0; f < fanCount_; ++f) {
        for (uint8_t b = 0; b < BUTTON_COUNT; ++b) {
            ButtonState& state = buttons_[f][b];
            state.pin = config.fans[f].buttonPins[b];
            if (state.pin < 0) continue;
            state.activeLow = config.fans[f].buttonActiveLow;
            pinMode(state.pin, state.activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
            state.lastRawPressed = state.activeLow ? digitalRead(state.pin) == LOW : digitalRead(state.pin) == HIGH;
            state.stablePressed = state.lastRawPressed;
            state.changedAt = millis();
        }
    }
}

void ButtonManager::service(uint32_t nowMs) {
    if (!commands_) return;
    for (uint8_t f = 0; f < fanCount_; ++f) {
        for (uint8_t b = 0; b < BUTTON_COUNT; ++b) {
            ButtonState& state = buttons_[f][b];
            if (state.pin < 0) continue;
            bool raw = state.activeLow ? digitalRead(state.pin) == LOW : digitalRead(state.pin) == HIGH;
            if (raw != state.lastRawPressed) {
                state.lastRawPressed = raw;
                state.changedAt = nowMs;
            }
            if (raw != state.stablePressed && static_cast<uint32_t>(nowMs - state.changedAt) >= 50) {
                state.stablePressed = raw;
                if (raw && !commands_->enqueue({f, static_cast<uint8_t>(b), CommandSource::Button})) {
                    Serial.printf("Button command dropped: queue full for fan %u.\n", f + 1);
                }
            }
        }
    }
}
