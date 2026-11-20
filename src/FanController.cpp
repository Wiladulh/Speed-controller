#include "FanController.h"

bool FanController::isValidProfile(const FanHardwareProfile& profile) {
    if (profile.availableSpeeds < 2 || profile.availableSpeeds > 4) {
        return false;
    }

    if (profile.relayActiveLevel != HIGH && profile.relayActiveLevel != LOW) {
        return false;
    }

    if (profile.deadTimeMs == 0) {
        return false;
    }

    for (uint8_t i = 0; i < profile.availableSpeeds; ++i) {
        if (profile.relayPins[i] < 0) {
            return false;
        }

        for (uint8_t j = i + 1; j < profile.availableSpeeds; ++j) {
            if (profile.relayPins[i] == profile.relayPins[j]) {
                return false;
            }
        }
    }

    return true;
}

bool FanController::begin(const FanHardwareProfile& profile) {
    configured_ = false;
    currentSpeed_ = 0;
    targetSpeed_ = 0;

    if (!isValidProfile(profile)) {
        state_ = FanRunState::Unconfigured;
        return false;
    }

    profile_ = profile;

    // Preload the inactive output level before switching each pin to OUTPUT.
    const int inactiveLevel = profile_.relayActiveLevel == HIGH ? LOW : HIGH;
    for (uint8_t i = 0; i < profile_.availableSpeeds; ++i) {
        const int pin = profile_.relayPins[i];
        digitalWrite(pin, inactiveLevel);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, inactiveLevel);
    }

    configured_ = true;
    state_ = FanRunState::Off;
    return true;
}

void FanController::allOutputsOff() {
    if (!configured_) {
        return;
    }

    const int inactiveLevel = profile_.relayActiveLevel == HIGH ? LOW : HIGH;
    for (uint8_t i = 0; i < profile_.availableSpeeds; ++i) {
        digitalWrite(profile_.relayPins[i], inactiveLevel);
    }
}

bool FanController::requestSpeed(uint8_t speed) {
    if (!configured_ || speed > profile_.availableSpeeds || state_ == FanRunState::Error) {
        return false;
    }

    if (speed == currentSpeed_ && state_ == FanRunState::Running) {
        return true;
    }

    if (speed == 0 && state_ == FanRunState::Off) {
        return true;
    }

    // Every new target begins from a fully de-energized output bank.
    allOutputsOff();
    currentSpeed_ = 0;
    targetSpeed_ = speed;

    if (speed == 0) {
        state_ = FanRunState::Off;
        return true;
    }

    transitionStartedAtMs_ = millis();
    state_ = FanRunState::Transition;
    return true;
}

void FanController::service(uint32_t nowMs) {
    if (!configured_ || state_ != FanRunState::Transition) {
        return;
    }

    if (static_cast<uint32_t>(nowMs - transitionStartedAtMs_) < profile_.deadTimeMs) {
        return;
    }

    allOutputsOff();
    const uint8_t relayIndex = static_cast<uint8_t>(targetSpeed_ - 1);
    digitalWrite(profile_.relayPins[relayIndex], profile_.relayActiveLevel);

    currentSpeed_ = targetSpeed_;
    state_ = FanRunState::Running;
}

uint8_t FanController::currentSpeed() const {
    return currentSpeed_;
}

uint8_t FanController::targetSpeed() const {
    return targetSpeed_;
}

FanRunState FanController::state() const {
    return state_;
}

bool FanController::isConfigured() const {
    return configured_;
}
