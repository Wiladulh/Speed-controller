#pragma once

#include <Arduino.h>
#include "Config.h"

class WifiManager {
public:
    void begin(const AppConfig& config);
    void applyConfig(const AppConfig& config);
    void service(uint32_t nowMs);
    bool stationConnected() const;

private:
    AppConfig config_{};
    uint32_t lastReconnectAttempt_ = 0;
    bool started_ = false;
};
