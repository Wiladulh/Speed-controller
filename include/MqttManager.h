#pragma once

#include <Arduino.h>
#include <PsychicMqttClient.h>
#include "Config.h"
#include "CommandManager.h"

class MqttManager {
public:
    void begin(const AppConfig& config, CommandManager& commands);
    void applyConfig(const AppConfig& config);
    bool connected() const;
    void publishFanState(const AppConfig& config, uint8_t fanIndex, uint8_t speed);
    void publishDeviceStatus(const AppConfig& config, const char* status);

private:
    PsychicMqttClient client_;
    AppConfig config_{};
    String uri_;
    String willTopic_;
    CommandManager* commands_ = nullptr;
    bool configured_ = false;
    bool callbacksRegistered_ = false;
    void configureClient();
    void handleMessage(char* topic, char* payload, int retain, int qos, bool dup);
    String topicFor(uint8_t fanIndex, const char* suffix) const;
};
