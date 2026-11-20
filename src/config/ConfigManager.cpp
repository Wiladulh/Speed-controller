#include "ConfigManager.h"

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_system.h>
#include <cstring>

#include "BoardProfile.h"

namespace {
Preferences preferences;

void copyText(char* target, size_t capacity, const char* source) {
    if (!capacity) return;
    strlcpy(target, source ? source : "", capacity);
}

const char* transportName(MqttTransport transport) {
    switch (transport) {
        case MqttTransport::Ws: return "ws";
        case MqttTransport::Wss: return "wss";
        case MqttTransport::Tcp: return "mqtt";
        case MqttTransport::Tls: return "mqtts";
    }
    return "wss";
}
}

bool ConfigManager::begin() {
    preferences.begin("fanctrl", false);
    if (!load()) {
        createInitialCredentials();
    }
    for (uint8_t i = 0; i < config_.fanCount; ++i) {
        if (!config_.fans[i].id[0]) {
            snprintf(config_.fans[i].id, sizeof(config_.fans[i].id), "fan-%03u", i + 1);
        }
    }
    save(config_);
    return true;
}

void ConfigManager::createInitialCredentials() {
    uint64_t mac = ESP.getEfuseMac();
    char suffix[13];
    snprintf(suffix, sizeof(suffix), "%012llX",
             static_cast<unsigned long long>(mac & 0xFFFFFFFFFFFFULL));

    char apName[32];
    snprintf(apName, sizeof(apName), "SpeedController-%s", suffix + 6);
    copyText(config_.apSsid, sizeof(config_.apSsid), apName);

    char deviceName[32];
    snprintf(deviceName, sizeof(deviceName), "fan-%s", suffix);
    copyText(config_.deviceId, sizeof(config_.deviceId), deviceName);

    char apKey[32];
    snprintf(apKey, sizeof(apKey), "Fan-%08lX",
             static_cast<unsigned long>(esp_random()));
    copyText(config_.apPassword, sizeof(config_.apPassword), apKey);

    char adminKey[48];
    snprintf(adminKey, sizeof(adminKey), "Admin-%08lX%08lX",
             static_cast<unsigned long>(esp_random()),
             static_cast<unsigned long>(esp_random()));
    copyText(config_.webPassword, sizeof(config_.webPassword), adminKey);

    Serial.printf("Initial AP SSID: %s\n", config_.apSsid);
    Serial.printf("Initial AP password: %s\n", config_.apPassword);
    Serial.printf("Initial WebUI username: %s\n", config_.webUsername);
    Serial.printf("Initial WebUI password: %s\n", config_.webPassword);
}
bool ConfigManager::load() {
    String json = preferences.getString("config", "");
    if (json.length() == 0) return false;
    AppConfig loaded{};
    if (!fromJson(json, loaded)) return false;
    String error;
    if (!validate(loaded, error)) {
        Serial.printf("Stored configuration rejected: %s\n", error.c_str());
        return false;
    }
    config_ = loaded;
    return true;
}

const AppConfig& ConfigManager::get() const {
    return config_;
}

bool ConfigManager::save(const AppConfig& config) {
    String error;
    if (!validate(config, error)) return false;
    JsonDocument doc;
    doc["schemaVersion"] = 1;
    doc["wifiSsid"] = config.wifiSsid;
    doc["wifiPassword"] = config.wifiPassword;
    doc["apSsid"] = config.apSsid;
    doc["apPassword"] = config.apPassword;
    doc["webUsername"] = config.webUsername;
    doc["webPassword"] = config.webPassword;
    doc["deviceId"] = config.deviceId;
    doc["mqttEnabled"] = config.mqttEnabled;
    doc["blePresenceEnabled"] = config.blePresenceEnabled;
    doc["mqttTransport"] = static_cast<uint8_t>(config.mqttTransport);
    doc["mqttHost"] = config.mqttHost;
    doc["mqttPort"] = config.mqttPort;
    doc["mqttPath"] = config.mqttPath;
    doc["mqttUsername"] = config.mqttUsername;
    doc["mqttPassword"] = config.mqttPassword;
    doc["mqttCaPem"] = config.mqttCaPem;
    doc["mqttBaseTopic"] = config.mqttBaseTopic;
    doc["registeredItag"] = config.registeredItag;
    doc["bleFanMask"] = config.bleFanMask;
    doc["presenceRssiThreshold"] = config.presenceRssiThreshold;
    doc["presenceTimeoutSeconds"] = config.presenceTimeoutSeconds;
    doc["fanCount"] = config.fanCount;
    JsonArray fans = doc["fans"].to<JsonArray>();
    for (uint8_t i = 0; i < config.fanCount; ++i) {
        JsonObject fan = fans.add<JsonObject>();
        fan["id"] = config.fans[i].id;
        fan["availableSpeeds"] = config.fans[i].availableSpeeds;
        fan["relayActiveLow"] = config.fans[i].relayActiveLow;
        fan["buttonActiveLow"] = config.fans[i].buttonActiveLow;
        fan["deadTimeMs"] = config.fans[i].deadTimeMs;
        JsonArray speedPins = fan["speedPins"].to<JsonArray>();
        JsonArray buttonPins = fan["buttonPins"].to<JsonArray>();
        for (uint8_t j = 0; j < MAX_SPEEDS; ++j) speedPins.add(config.fans[i].speedPins[j]);
        for (uint8_t j = 0; j < BUTTON_COUNT; ++j) buttonPins.add(config.fans[i].buttonPins[j]);
    }
    String serialized;
    serializeJson(doc, serialized);
    if (preferences.putString("config", serialized) == 0) return false;
    config_ = config;
    config_.schemaVersion = 1;
    return true;
}

String ConfigManager::toJson() const {
    JsonDocument doc;
    doc["schemaVersion"] = config_.schemaVersion;
    doc["wifiSsid"] = config_.wifiSsid;
    doc["apSsid"] = config_.apSsid;
    doc["webUsername"] = config_.webUsername;
    doc["deviceId"] = config_.deviceId;
    doc["mqttEnabled"] = config_.mqttEnabled;
    doc["blePresenceEnabled"] = config_.blePresenceEnabled;
    doc["mqttTransport"] = static_cast<uint8_t>(config_.mqttTransport);
    doc["mqttHost"] = config_.mqttHost;
    doc["mqttPort"] = config_.mqttPort;
    doc["mqttPath"] = config_.mqttPath;
    doc["mqttUsername"] = config_.mqttUsername;
    doc["mqttBaseTopic"] = config_.mqttBaseTopic;
    doc["registeredItag"] = config_.registeredItag;
    doc["bleFanMask"] = config_.bleFanMask;
    doc["presenceRssiThreshold"] = config_.presenceRssiThreshold;
    doc["presenceTimeoutSeconds"] = config_.presenceTimeoutSeconds;
    doc["fanCount"] = config_.fanCount;
    JsonArray fans = doc["fans"].to<JsonArray>();
    for (uint8_t i = 0; i < config_.fanCount; ++i) {
        JsonObject fan = fans.add<JsonObject>();
        fan["id"] = config_.fans[i].id;
        fan["availableSpeeds"] = config_.fans[i].availableSpeeds;
        fan["relayActiveLow"] = config_.fans[i].relayActiveLow;
        fan["buttonActiveLow"] = config_.fans[i].buttonActiveLow;
        fan["deadTimeMs"] = config_.fans[i].deadTimeMs;
        JsonArray speedPins = fan["speedPins"].to<JsonArray>();
        JsonArray buttonPins = fan["buttonPins"].to<JsonArray>();
        for (uint8_t j = 0; j < MAX_SPEEDS; ++j) speedPins.add(config_.fans[i].speedPins[j]);
        for (uint8_t j = 0; j < BUTTON_COUNT; ++j) buttonPins.add(config_.fans[i].buttonPins[j]);
    }
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::fromJson(const String& json, AppConfig& output) const {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return false;
    output = AppConfig{};
    output.schemaVersion = doc["schemaVersion"] | 1;
    copyText(output.wifiSsid, sizeof(output.wifiSsid), doc["wifiSsid"] | "");
    copyText(output.wifiPassword, sizeof(output.wifiPassword), doc["wifiPassword"] | "");
    copyText(output.apSsid, sizeof(output.apSsid), doc["apSsid"] | "SpeedController-Setup");
    copyText(output.apPassword, sizeof(output.apPassword), doc["apPassword"] | "");
    copyText(output.webUsername, sizeof(output.webUsername), doc["webUsername"] | "admin");
    copyText(output.webPassword, sizeof(output.webPassword), doc["webPassword"] | "");
    copyText(output.deviceId, sizeof(output.deviceId), doc["deviceId"] | "esp32-fan-controller");
    output.mqttEnabled = doc["mqttEnabled"] | false;
    output.blePresenceEnabled = doc["blePresenceEnabled"] | false;
    uint8_t transport = doc["mqttTransport"] | 1;
    if (transport > 3) return false;
    output.mqttTransport = static_cast<MqttTransport>(transport);
    copyText(output.mqttHost, sizeof(output.mqttHost), doc["mqttHost"] | "");
    output.mqttPort = doc["mqttPort"] | 443;
    copyText(output.mqttPath, sizeof(output.mqttPath), doc["mqttPath"] | "/mqtt");
    copyText(output.mqttUsername, sizeof(output.mqttUsername), doc["mqttUsername"] | "");
    copyText(output.mqttPassword, sizeof(output.mqttPassword), doc["mqttPassword"] | "");
    copyText(output.mqttCaPem, sizeof(output.mqttCaPem), doc["mqttCaPem"] | "");
    copyText(output.mqttBaseTopic, sizeof(output.mqttBaseTopic), doc["mqttBaseTopic"] | "devices");
    copyText(output.registeredItag, sizeof(output.registeredItag), doc["registeredItag"] | "");
    output.bleFanMask = doc["bleFanMask"] | 1;
    output.presenceRssiThreshold = doc["presenceRssiThreshold"] | -70;
    output.presenceTimeoutSeconds = doc["presenceTimeoutSeconds"] | 30;
    output.fanCount = doc["fanCount"] | 1;
    if (output.fanCount < 1 || output.fanCount > MAX_FANS) return false;
    JsonArrayConst fans = doc["fans"].as<JsonArrayConst>();
    for (uint8_t i = 0; i < output.fanCount; ++i) {
        if (i >= fans.size()) continue;
        JsonObjectConst fan = fans[i];
        if (fan["id"].is<const char*>()) {
            copyText(output.fans[i].id, sizeof(output.fans[i].id), fan["id"]);
        } else {
            snprintf(output.fans[i].id, sizeof(output.fans[i].id), "fan-%03u", i + 1);
        }
        output.fans[i].availableSpeeds = fan["availableSpeeds"] | 4;
        output.fans[i].relayActiveLow = fan["relayActiveLow"] | true;
        output.fans[i].buttonActiveLow = fan["buttonActiveLow"] | true;
        output.fans[i].deadTimeMs = fan["deadTimeMs"] | 1000;
        JsonArrayConst speedPins = fan["speedPins"].as<JsonArrayConst>();
        JsonArrayConst buttonPins = fan["buttonPins"].as<JsonArrayConst>();
        for (uint8_t j = 0; j < MAX_SPEEDS; ++j) output.fans[i].speedPins[j] = j < speedPins.size() ? speedPins[j].as<int8_t>() : -1;
        for (uint8_t j = 0; j < BUTTON_COUNT; ++j) output.fans[i].buttonPins[j] = j < buttonPins.size() ? buttonPins[j].as<int8_t>() : -1;
    }
    return output.schemaVersion == 1;
}

bool ConfigManager::validate(const AppConfig& config, String& error) const {
    if (config.fanCount < 1 || config.fanCount > MAX_FANS) { error = "Fan count must be 1-8."; return false; }
    if (config.apPassword[0] && strlen(config.apPassword) < 8) { error = "AP password must contain at least 8 characters."; return false; }
    if (!config.webUsername[0] || strlen(config.webPassword) < 8) { error = "WebUI credentials are required (password >= 8 characters)."; return false; }
    if (config.mqttPort == 0) { error = "MQTT port is invalid."; return false; }
    if (static_cast<uint8_t>(config.mqttTransport) > 3) { error = "MQTT transport is invalid."; return false; }
    const size_t baseTopicLength = strlen(config.mqttBaseTopic);
    if (!baseTopicLength || config.mqttBaseTopic[0] == '/' ||
        config.mqttBaseTopic[baseTopicLength - 1] == '/' ||
        strchr(config.mqttBaseTopic, '+') || strchr(config.mqttBaseTopic, '#')) {
        error = "MQTT base topic is invalid.";
        return false;
    }
    if (!config.deviceId[0] || strchr(config.deviceId, '/') || strchr(config.deviceId, '+') || strchr(config.deviceId, '#')) { error = "Device ID is invalid for MQTT topics."; return false; }
    if (config.presenceRssiThreshold < -100 || config.presenceRssiThreshold > -30) { error = "Presence RSSI threshold must be -100 to -30 dBm."; return false; }
    if ((config.bleFanMask & ~((1U << config.fanCount) - 1U)) != 0) { error = "BLE fan mapping refers to an unconfigured fan."; return false; }
    if (config.presenceTimeoutSeconds < 5 || config.presenceTimeoutSeconds > 600) { error = "Presence timeout must be 5-600 seconds."; return false; }

    bool used[64] = {};
    for (uint8_t i = 0; i < config.fanCount; ++i) {
        const FanConfig& fan = config.fans[i];
        if (!fan.id[0] || strchr(fan.id, '/') || strchr(fan.id, '+') || strchr(fan.id, '#')) { error = "Fan ID is empty or contains MQTT topic separators."; return false; }
        for (uint8_t prior = 0; prior < i; ++prior) {
            if (strcmp(config.fans[prior].id, fan.id) == 0) { error = "Fan IDs must be unique."; return false; }
        }
        if (fan.availableSpeeds < 2 || fan.availableSpeeds > 4) { error = "Each fan must use 2-4 speeds."; return false; }
        if (fan.deadTimeMs < 100 || fan.deadTimeMs > 10000) { error = "Dead time must be 100-10000 ms."; return false; }
        for (uint8_t j = 0; j < fan.availableSpeeds; ++j) {
            int pin = fan.speedPins[j];
            if (pin < 0) continue;
            if (!BoardProfile::isAllowedOutput(pin) || used[pin]) { error = "Speed GPIO is unavailable or duplicated."; return false; }
            used[pin] = true;
        }
        for (uint8_t j = 0; j < BUTTON_COUNT; ++j) {
            int pin = fan.buttonPins[j];
            if (pin < 0) continue;
            if (!BoardProfile::isAllowedInput(pin) || used[pin]) { error = "Button GPIO is unavailable or duplicated."; return false; }
            used[pin] = true;
        }
    }
    return true;
}

String ConfigManager::mqttUri(const AppConfig& config) const {
    String scheme = transportName(config.mqttTransport);
    String uri = scheme + "://";
    if (config.mqttUsername[0]) uri += String(config.mqttUsername) + ":" + String(config.mqttPassword) + "@";
    uri += config.mqttHost;
    uri += ":" + String(config.mqttPort);
    if (config.mqttTransport == MqttTransport::Ws || config.mqttTransport == MqttTransport::Wss) {
        uri += config.mqttPath[0] == '/' ? String(config.mqttPath) : "/" + String(config.mqttPath);
    }
    return uri;
}
