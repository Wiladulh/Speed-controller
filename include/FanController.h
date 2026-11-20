#pragma once

#include <Arduino.h>
#include <stdint.h>

enum class FanRunState : uint8_t {
    Unconfigured,
    Off,
    Transition,
    Running,
    Error,
};

struct FanHardwareProfile {
    int relayPins[4] = {-1, -1, -1, -1};
    uint8_t availableSpeeds = 0;
    int relayActiveLevel = -1;
    uint32_t deadTimeMs = 0;
};

class FanController {
public:
    bool begin(const FanHardwareProfile& profile);
    bool requestSpeed(uint8_t speed);
    void service(uint32_t nowMs);

    uint8_t currentSpeed() const;
    uint8_t targetSpeed() const;
    FanRunState state() const;
    bool isConfigured() const;

private:
    static bool isValidProfile(const FanHardwareProfile& profile);
    void allOutputsOff();

    FanHardwareProfile profile_{};
    FanRunState state_ = FanRunState::Unconfigured;
    uint8_t currentSpeed_ = 0;
    uint8_t targetSpeed_ = 0;
    uint32_t transitionStartedAtMs_ = 0;
    bool configured_ = false;
};
