#include <Arduino.h>
#include <WiFi.h>

#include "BleRemoteManager.h"
#include "ButtonManager.h"
#include "CommandManager.h"
#include "ConfigManager.h"
#include "FanController.h"
#include "MqttManager.h"
#include "WebServerManager.h"
#include "WifiManager.h"

namespace {
ConfigManager configManager;
CommandManager commandManager;
MqttManager mqttManager;
BleRemoteManager bleManager;
WifiManager wifiManager;
WebServerManager webServer;
FanController fanControllers[MAX_FANS];
ButtonManager buttonManager;
uint8_t lastPublishedSpeed[MAX_FANS] = {255, 255, 255, 255, 255, 255, 255, 255};
uint32_t lastMqttConnectionEpoch = 0;
}

void setup() {
    Serial.begin(115200);
    configManager.begin();
    const AppConfig& config = configManager.get();

    commandManager.begin();
    wifiManager.begin(config);

    for (uint8_t i = 0; i < config.fanCount; ++i) {
        FanHardwareProfile profile{};
        profile.availableSpeeds = config.fans[i].availableSpeeds;
        profile.relayActiveLevel = config.fans[i].relayActiveLow ? LOW : HIGH;
        profile.deadTimeMs = config.fans[i].deadTimeMs;
        for (uint8_t speed = 0; speed < MAX_SPEEDS; ++speed) {
            profile.relayPins[speed] = config.fans[i].speedPins[speed];
        }
        if (!fanControllers[i].begin(profile)) {
            Serial.printf("Fan %u locked: GPIO profile incomplete or invalid.\n", i + 1);
        }
    }

    buttonManager.begin(config, commandManager);
    bleManager.begin(config, commandManager);
    mqttManager.begin(config, commandManager);
    webServer.begin(configManager, mqttManager, bleManager, commandManager, fanControllers);

    Serial.printf("Speed Controller AP: %s\n", WiFi.softAPIP().toString().c_str());
}

void loop() {
    const uint32_t now = millis();
    const AppConfig& config = configManager.get();

    wifiManager.service(now);
    buttonManager.service(now);
    bleManager.service(now);
    webServer.service();

    FanCommand command{};
    FanCommand selected[MAX_FANS]{};
    int selectedPriority[MAX_FANS];
    for (uint8_t i = 0; i < MAX_FANS; ++i) selectedPriority[i] = -1;
    auto priority = [](CommandSource source) {
        switch (source) {
            case CommandSource::Button: return 4;
            case CommandSource::WebUi: return 3;
            case CommandSource::Mqtt: return 3;
            case CommandSource::Ble: return 2;
            case CommandSource::Presence: return 1;
        }
        return 0;
    };
    while (commandManager.dequeue(command)) {
        if (command.fanIndex >= config.fanCount) continue;
        const int commandPriority = priority(command.source);
        if (commandPriority >= selectedPriority[command.fanIndex]) {
            selected[command.fanIndex] = command;
            selectedPriority[command.fanIndex] = commandPriority;
        }
    }
    for (uint8_t i = 0; i < config.fanCount; ++i) {
        if (selectedPriority[i] < 0 || !fanControllers[i].isConfigured()) continue;
        if (selected[i].source != CommandSource::Presence) bleManager.noteManualOverride();
        if (!fanControllers[i].requestSpeed(selected[i].speed)) {
            Serial.printf("Command rejected by fan %u controller: speed %u.\n",
                          i + 1, selected[i].speed);
        }
    }

    for (uint8_t i = 0; i < config.fanCount; ++i) {
        fanControllers[i].service(now);
        const FanRunState state = fanControllers[i].state();
        if (state == FanRunState::Off || state == FanRunState::Running) {
            const uint8_t speed = fanControllers[i].currentSpeed();
            if (lastPublishedSpeed[i] != speed) {
                mqttManager.publishFanState(config, i, speed);
                lastPublishedSpeed[i] = speed;
            }
        }
    }

    const uint32_t connectionEpoch = mqttManager.connectionEpoch();
    if (mqttManager.connected() && connectionEpoch != lastMqttConnectionEpoch) {
        for (uint8_t i = 0; i < config.fanCount; ++i) {
            if (fanControllers[i].isConfigured()) {
                mqttManager.publishFanState(config, i, fanControllers[i].currentSpeed());
            }
        }
        lastMqttConnectionEpoch = connectionEpoch;
    }
}
