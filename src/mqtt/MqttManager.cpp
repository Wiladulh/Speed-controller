#include "MqttManager.h"

#include <ArduinoJson.h>
#include <cstring>

extern const uint8_t rootca_crt_bundle_start[] asm("_binary_src_certs_x509_crt_bundle_bin_start");
extern const uint8_t rootca_crt_bundle_end[] asm("_binary_src_certs_x509_crt_bundle_bin_end");

void MqttManager::begin(const AppConfig& config, CommandManager& commands) {
    commands_ = &commands;
    applyConfig(config);
}

void MqttManager::applyConfig(const AppConfig& config) {
    bool changed = !configured_ ||
        strcmp(config_.mqttHost, config.mqttHost) != 0 ||
        config_.mqttPort != config.mqttPort ||
        config_.mqttTransport != config.mqttTransport ||
        strcmp(config_.mqttPath, config.mqttPath) != 0 ||
        strcmp(config_.mqttUsername, config.mqttUsername) != 0 ||
        strcmp(config_.mqttPassword, config.mqttPassword) != 0 ||
        config_.mqttEnabled != config.mqttEnabled ||
        strcmp(config_.mqttCaPem, config.mqttCaPem) != 0 ||
        strcmp(config_.mqttBaseTopic, config.mqttBaseTopic) != 0 ||
        strcmp(config_.deviceId, config.deviceId) != 0 ||
        config_.fanCount != config.fanCount;
    if (!changed) {
        for (uint8_t i = 0; i < config.fanCount; ++i) {
            if (strcmp(config_.fans[i].id, config.fans[i].id) != 0) {
                changed = true;
                break;
            }
        }
    }
    config_ = config;
    if (changed) configureClient();
}

void MqttManager::configureClient() {
    if (configured_) client_.forceStop();

    if (!callbacksRegistered_) {
        client_.onConnect([this](bool) {
            ++connectionEpoch_;
            if (!config_.mqttEnabled) return;
            for (uint8_t i = 0; i < config_.fanCount; ++i) {
                String filter = topicFor(i, "set");
                // Exact subscriptions are created for each configured fan.
                client_.subscribe(filter.c_str(), 1);
            }
            publishDeviceStatus(config_, "online");
        });
        client_.onDisconnect([](bool) {});
        client_.onMessage([this](char* topic, char* payload, int retain, int qos, bool dup) {
            handleMessage(topic, payload, retain, qos, dup);
        });
        callbacksRegistered_ = true;
    }

    configured_ = true;
    if (!config_.mqttEnabled || !config_.mqttHost[0]) return;

    const bool secure = config_.mqttTransport == MqttTransport::Wss ||
                        config_.mqttTransport == MqttTransport::Tls;
    uri_ = String(
        config_.mqttTransport == MqttTransport::Ws ? "ws://" :
        config_.mqttTransport == MqttTransport::Wss ? "wss://" :
        config_.mqttTransport == MqttTransport::Tcp ? "mqtt://" : "mqtts://"
    ) + config_.mqttHost + ":" + String(config_.mqttPort);
    if (config_.mqttTransport == MqttTransport::Ws || config_.mqttTransport == MqttTransport::Wss) {
        uri_ += config_.mqttPath[0] == '/' ? String(config_.mqttPath) : "/" + String(config_.mqttPath);
    }

    client_.setServer(uri_.c_str());
    client_.setClientId(config_.deviceId);
    client_.setKeepAlive(45);
    client_.setAutoReconnect(true);
    client_.setCleanSession(true);
    if (config_.mqttUsername[0]) client_.setCredentials(config_.mqttUsername, config_.mqttPassword);
    else client_.setCredentials(nullptr, nullptr);
    if (secure) {
        if (config_.mqttCaPem[0]) {
            // Preserve support for a broker-specific CA supplied by the user.
            client_.setCACert(config_.mqttCaPem);
        } else {
            // Use the PlatformIO-embedded public CA bundle; TLS verification stays enabled.
            client_.setCACertBundle(
                rootca_crt_bundle_start,
                static_cast<size_t>(rootca_crt_bundle_end - rootca_crt_bundle_start)
            );
        }
    }

    willTopic_ = String(config_.mqttBaseTopic) + "/" + config_.deviceId + "/status";
    client_.setWill(willTopic_.c_str(), 0, true, "offline");
    client_.connect();
}

void MqttManager::handleMessage(char* topic, char* payload, int retain, int, bool) {
    if (!config_.mqttEnabled || !commands_ || retain != 0 || !topic || !payload) return;
    String fullTopic(topic);
    String prefix = String(config_.mqttBaseTopic) + "/" + config_.deviceId + "/";
    if (!fullTopic.startsWith(prefix) || !fullTopic.endsWith("/set")) return;
    String fanId = fullTopic.substring(prefix.length(), fullTopic.length() - 4);
    int fanIndex = -1;
    for (uint8_t i = 0; i < config_.fanCount; ++i) {
        if (fanId == config_.fans[i].id) fanIndex = i;
    }
    if (fanIndex < 0) return;

    JsonDocument doc;
    if (deserializeJson(doc, payload) || !doc["speed"].is<uint8_t>()) return;
    uint8_t speed = doc["speed"].as<uint8_t>();
    if (speed > config_.fans[fanIndex].availableSpeeds) return;
    if (!commands_->enqueue({static_cast<uint8_t>(fanIndex), speed, CommandSource::Mqtt})) {
        Serial.printf("MQTT command dropped: queue full for fan %s.\n", fanId.c_str());
    }
}

bool MqttManager::connected() const {
    return const_cast<PsychicMqttClient&>(client_).connected();
}

uint32_t MqttManager::connectionEpoch() const {
    return connectionEpoch_;
}

String MqttManager::topicFor(uint8_t fanIndex, const char* suffix) const {
    if (fanIndex >= config_.fanCount) return "";
    return String(config_.mqttBaseTopic) + "/" + config_.deviceId + "/" + config_.fans[fanIndex].id + "/" + suffix;
}

void MqttManager::publishFanState(const AppConfig& config, uint8_t fanIndex, uint8_t speed) {
    if (!config_.mqttEnabled || !client_.connected() || fanIndex >= config.fanCount) return;
    JsonDocument doc;
    doc["fan_id"] = config.fans[fanIndex].id;
    doc["power"] = speed > 0;
    doc["speed"] = speed;
    String payload;
    serializeJson(doc, payload);
    String topic = topicFor(fanIndex, "state");
    client_.publish(topic.c_str(), 1, true, payload.c_str());
}

void MqttManager::publishDeviceStatus(const AppConfig& config, const char* status) {
    if (!config_.mqttEnabled || !client_.connected()) return;
    String topic = String(config_.mqttBaseTopic) + "/" + config.deviceId + "/status";
    client_.publish(topic.c_str(), 1, true, status);
}
