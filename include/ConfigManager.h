#pragma once

#include <Arduino.h>
#include "Config.h"

class ConfigManager {
public:
    bool begin();
    const AppConfig& get() const;
    bool save(const AppConfig& config);
    String toJson() const;
    bool fromJson(const String& json, AppConfig& output) const;
    bool validate(const AppConfig& config, String& error) const;
    String mqttUri(const AppConfig& config) const;

private:
    AppConfig config_{};
    bool load();
    void createInitialCredentials();
};
