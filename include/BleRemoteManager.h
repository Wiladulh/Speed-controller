#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "Config.h"
#include "CommandManager.h"
#include "ItagClickSequence.h"

class BleRemoteManager {
public:
    void begin(const AppConfig& config, CommandManager& commands);
    void applyConfig(const AppConfig& config);
    String scanResultsJson() const;
    void setPresenceEnabled(bool enabled);
    void service(uint32_t nowMs);
    void noteManualOverride();
    void feedButtonEdge(bool pressed, uint32_t nowMs);
    bool isPresent() const;

private:
    class ScanCallbacks : public NimBLEScanCallbacks {
    public:
        explicit ScanCallbacks(BleRemoteManager* owner) : owner_(owner) {}
        void onResult(const NimBLEAdvertisedDevice* device) override;
    private:
        BleRemoteManager* owner_;
    };

    struct SeenDevice {
        String address;
        String name;
        int rssi = -127;
        String manufacturer;
        uint32_t seenAt = 0;
    };

    AppConfig config_{};
    CommandManager* commands_ = nullptr;
    ScanCallbacks callbacks_{this};
    NimBLEScan* scanner_ = nullptr;
    SeenDevice devices_[12]{};
    uint8_t deviceCount_ = 0;
    uint32_t lastSeenAt_ = 0;
    int smoothedRssi_ = -127;
    bool present_ = false;
    bool absenceCommandSent_ = false;
    ItagClickSequence clickSequence_;
    void onAdvertisement(const NimBLEAdvertisedDevice* device);
};
