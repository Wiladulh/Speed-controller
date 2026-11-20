#include "BleRemoteManager.h"

#include <ArduinoJson.h>
#include <cstring>

void BleRemoteManager::ScanCallbacks::onResult(const NimBLEAdvertisedDevice* device) {
    if (owner_) owner_->onAdvertisement(device);
}

void BleRemoteManager::begin(const AppConfig& config, CommandManager& commands) {
    commands_ = &commands;
    applyConfig(config);
    NimBLEDevice::init("SpeedController");
    scanner_ = NimBLEDevice::getScan();
    scanner_->setScanCallbacks(&callbacks_, true);
    scanner_->setActiveScan(true);
    scanner_->setInterval(80);
    scanner_->setWindow(60);
    scanner_->start(0, false);
}

void BleRemoteManager::applyConfig(const AppConfig& config) {
    bool wasEnabled = config_.blePresenceEnabled;
    bool remoteChanged = strcmp(config_.registeredItag, config.registeredItag) != 0;
    config_ = config;
    if (remoteChanged) {
        present_ = false;
        hasBeenPresent_ = false;
        smoothedRssi_ = -127;
        lastSeenAt_ = millis();
        absenceCommandSent_ = false;
    }
    if (wasEnabled != config_.blePresenceEnabled) setPresenceEnabled(config_.blePresenceEnabled);
}

void BleRemoteManager::setPresenceEnabled(bool enabled) {
    config_.blePresenceEnabled = enabled;
    if (!enabled) {
        present_ = false;
        hasBeenPresent_ = false;
        absenceCommandSent_ = false;
    } else {
        lastSeenAt_ = millis();
        present_ = false;
        hasBeenPresent_ = false;
        absenceCommandSent_ = false;
    }
}

void BleRemoteManager::noteManualOverride() {
    // Manual control is authoritative while the remote remains absent.
    // Presence automation re-arms only after a fresh near detection.
    if (!present_) absenceCommandSent_ = true;
}

bool BleRemoteManager::isPresent() const {
    return present_;
}

void BleRemoteManager::onAdvertisement(const NimBLEAdvertisedDevice* device) {
    if (!device) return;
    String address = device->getAddress().toString().c_str();
    String name = device->haveName() ? String(device->getName().c_str()) : "";
    String manufacturer;
    if (device->haveManufacturerData()) {
        const std::string raw = device->getManufacturerData();
        char byteHex[3];
        for (unsigned char value : raw) {
            snprintf(byteHex, sizeof(byteHex), "%02X", value);
            manufacturer += byteHex;
        }
    }
    int rssi = device->getRSSI();

    int index = -1;
    for (uint8_t i = 0; i < deviceCount_; ++i) {
        if (devices_[i].address == address) index = i;
    }
    if (index < 0) {
        if (deviceCount_ < 12) index = deviceCount_++;
        else index = 0;
    }
    devices_[index] = {address, name, rssi, manufacturer, millis()};

    if (config_.registeredItag[0] && address.equalsIgnoreCase(config_.registeredItag)) {
        smoothedRssi_ = smoothedRssi_ == -127 ? rssi : (smoothedRssi_ * 3 + rssi) / 4;
        const int exitThreshold = static_cast<int>(config_.presenceRssiThreshold) - 5;
        const bool near = present_ ? smoothedRssi_ >= exitThreshold
                                   : smoothedRssi_ >= config_.presenceRssiThreshold;
        if (near) {
            lastSeenAt_ = millis();
            present_ = true;
            hasBeenPresent_ = true;
            absenceCommandSent_ = false;
        }
    }
}

void BleRemoteManager::feedButtonEdge(bool pressed, uint32_t nowMs) {
    clickSequence_.feedButtonEdge(pressed, nowMs);
}

void BleRemoteManager::service(uint32_t nowMs) {
    if (!commands_) return;
    const uint8_t clicks = clickSequence_.service(nowMs);
    if (clicks > 0 && clicks <= 5) {
        const uint8_t speed = clicks == 5 ? 0 : clicks;
        for (uint8_t i = 0; i < config_.fanCount; ++i) {
            if (config_.bleFanMask & (1U << i)) {
                commands_->enqueue({i, speed, CommandSource::Ble});
            }
        }
    }
    if (!config_.blePresenceEnabled || !config_.registeredItag[0] || !hasBeenPresent_) return;
    if (!absenceCommandSent_ &&
        static_cast<uint32_t>(nowMs - lastSeenAt_) >= config_.presenceTimeoutSeconds * 1000UL) {
        present_ = false;
        absenceCommandSent_ = true;
        for (uint8_t i = 0; i < config_.fanCount; ++i) {
            if (config_.bleFanMask & (1U << i)) {
                commands_->enqueue({i, 0, CommandSource::Presence});
            }
        }
    }
}

String BleRemoteManager::scanResultsJson() const {
    JsonDocument doc;
    JsonArray list = doc.to<JsonArray>();
    for (uint8_t i = 0; i < deviceCount_; ++i) {
        JsonObject item = list.add<JsonObject>();
        item["address"] = devices_[i].address;
        item["name"] = devices_[i].name;
        item["rssi"] = devices_[i].rssi;
        item["manufacturer"] = devices_[i].manufacturer;
    }
    String output;
    serializeJson(doc, output);
    return output;
}
