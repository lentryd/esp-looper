#pragma once
#include "Event.h"
#include "Task.h"
#include <vector>
#include <memory>

namespace ESPLooper {

class Looper {
public:
    static Looper& getInstance();
    
    // Initialize the framework
    void begin(UBaseType_t dispatcherPriority = 3, BaseType_t dispatcherCore = 1);
    
    // Add tasks
    void addTask(std::shared_ptr<Task> task);
    void removeTask(std::shared_ptr<Task> task);
    void removeTask(const char* name);
    
    // Create timer task
    std::shared_ptr<TimerTask> addTimer(
        const char* name,
        Task::TaskCallback callback,
        uint32_t periodMs,
        bool autoStart = true,
        BaseType_t coreId = tskNO_AFFINITY,
        uint32_t stackSize = 4096,
        UBaseType_t priority = 1
    );
    
    // Create event listener
    std::shared_ptr<ListenerTask> addListener(
        const char* name,
        uint32_t eventId,
        ListenerTask::EventCallback callback,
        BaseType_t coreId = tskNO_AFFINITY,
        uint32_t stackSize = 4096,
        UBaseType_t priority = 1
    );
    
    // Event system access
    EventBus& events() { return EventBus::getInstance(); }
    
    // Send event
    bool sendEvent(uint32_t eventId, void* data = nullptr, size_t dataSize = 0, bool copyData = false);
    bool sendEvent(const char* eventName, void* data = nullptr, size_t dataSize = 0, bool copyData = false);
    
    // Task management
    std::shared_ptr<Task> getTask(const char* name);
    size_t getTaskCount() const;
    
    // Statistics
    void printStats() const;
    
private:
    Looper();
    ~Looper();
    
    std::vector<std::shared_ptr<Task>> tasks;
    SemaphoreHandle_t tasksMutex;
    TaskHandle_t eventDispatcherHandle;
    bool initialized;
    
    static void eventDispatcherTask(void* parameter);
};

} // namespace ESPLooper

// Global convenience macros
#define ESP_LOOPER ESPLooper::Looper::getInstance()

#define ESP_TIMER(name, period, callback, ...) \
    ESP_LOOPER.addTimer(name, callback, period, ##__VA_ARGS__)

#define ESP_LISTENER(name, eventId, callback, ...) \
    ESP_LOOPER.addListener(name, eventId, callback, ##__VA_ARGS__)

#define ESP_SEND_EVENT(eventId, data, size) \
    ESP_LOOPER.sendEvent(eventId, data, size, true)

#define ESP_SEND_EVENT_REF(eventId, data, size) \
    ESP_LOOPER.sendEvent(eventId, data, size, false)