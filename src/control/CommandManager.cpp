#include "CommandManager.h"

bool CommandManager::begin() {
    queue_ = xQueueCreate(16, sizeof(FanCommand));
    return queue_ != nullptr;
}

bool CommandManager::enqueue(const FanCommand& command) {
    if (!queue_) return false;
    return xQueueSend(queue_, &command, 0) == pdTRUE;
}

bool CommandManager::dequeue(FanCommand& command) {
    if (!queue_) return false;
    return xQueueReceive(queue_, &command, 0) == pdTRUE;
}
