#include "WifiManager.h"

#include <WiFi.h>
#include <cstring>

void WifiManager::begin(const AppConfig& config) {
    applyConfig(config);
}

void WifiManager::applyConfig(const AppConfig& config) {
    bool changed = !started_ ||
        strcmp(config_.wifiSsid, config.wifiSsid) != 0 ||
        strcmp(config_.wifiPassword, config.wifiPassword) != 0 ||
        strcmp(config_.apSsid, config.apSsid) != 0 ||
        strcmp(config_.apPassword, config.apPassword) != 0;
    config_ = config;
    if (!changed) return;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(config_.apSsid, config_.apPassword);
    if (config_.wifiSsid[0]) {
        WiFi.begin(config_.wifiSsid, config_.wifiPassword);
        lastReconnectAttempt_ = millis();
    } else {
        WiFi.disconnect(false, false);
    }
    started_ = true;
}

void WifiManager::service(uint32_t nowMs) {
    if (!config_.wifiSsid[0] || WiFi.status() == WL_CONNECTED) return;
    if (static_cast<uint32_t>(nowMs - lastReconnectAttempt_) < 15000) return;
    lastReconnectAttempt_ = nowMs;
    WiFi.disconnect(false, false);
    WiFi.begin(config_.wifiSsid, config_.wifiPassword);
}

bool WifiManager::stationConnected() const {
    return WiFi.status() == WL_CONNECTED;
}
