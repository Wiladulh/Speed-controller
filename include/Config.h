#pragma once

#include <Arduino.h>
#include <stdint.h>

constexpr uint8_t MAX_FANS = 8;
constexpr uint8_t MAX_SPEEDS = 4;
constexpr uint8_t BUTTON_COUNT = 5;
constexpr size_t MAX_TEXT = 128;

enum class MqttTransport : uint8_t { Ws, Wss, Tcp, Tls };

struct FanConfig {
    char id[24] = "";
    uint8_t availableSpeeds = 4;
    int8_t speedPins[MAX_SPEEDS] = {-1, -1, -1, -1};
    int8_t buttonPins[BUTTON_COUNT] = {-1, -1, -1, -1, -1};
    bool relayActiveLow = true;
    bool buttonActiveLow = true;
    uint16_t deadTimeMs = 1000;
};

struct AppConfig {
    uint16_t schemaVersion = 1;
    char wifiSsid[64] = "";
    char wifiPassword[64] = "";
    char apSsid[32] = "SpeedController-Setup";
    char apPassword[32] = "";
    char webUsername[32] = "admin";
    char webPassword[48] = "";
    char deviceId[32] = "esp32-fan-controller";
    bool mqttEnabled = false;
    bool blePresenceEnabled = false;
    MqttTransport mqttTransport = MqttTransport::Wss;
    char mqttHost[96] = "";
    uint16_t mqttPort = 443;
    char mqttPath[64] = "/mqtt";
    char mqttUsername[64] = "";
    char mqttPassword[64] = "";
    char mqttCaPem[2048] = "";
    char mqttBaseTopic[96] = "devices";
    char registeredItag[24] = "";
    uint8_t bleFanMask = 1;
    int8_t presenceRssiThreshold = -70;
    uint16_t presenceTimeoutSeconds = 30;
    uint8_t fanCount = 1;
    FanConfig fans[MAX_FANS]{};
};
