#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "ConfigManager.h"
#include "MqttManager.h"
#include "BleRemoteManager.h"
#include "CommandManager.h"
#include "FanController.h"

class WebServerManager {
public:
    void begin(ConfigManager& config, MqttManager& mqtt, BleRemoteManager& ble,
               CommandManager& commands, FanController* fans);
    void service();

private:
    WebServer server_{80};
    ConfigManager* config_ = nullptr;
    MqttManager* mqtt_ = nullptr;
    BleRemoteManager* ble_ = nullptr;
    CommandManager* commands_ = nullptr;
    FanController* fans_ = nullptr;

    bool authenticate();
    void handleConfigGet();
    void handleConfigPost();
    void handleGpioOptions();
    void handleBleRegister();
    void handleFanCommand();
};
