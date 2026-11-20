#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class CommandSource : uint8_t { WebUi, Mqtt, Ble, Button, Presence };

struct FanCommand {
    uint8_t fanIndex;
    uint8_t speed;
    CommandSource source;
};

class CommandManager {
public:
    bool begin();
    bool enqueue(const FanCommand& command);
    bool dequeue(FanCommand& command);

private:
    QueueHandle_t queue_ = nullptr;
};
