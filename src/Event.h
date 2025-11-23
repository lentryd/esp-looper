#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <functional>
#include <map>
#include <vector>

namespace ESPLooper {

struct Event {
    uint32_t id;           // Event ID
    void* data;            // Event data pointer
    size_t dataSize;       // Size of data
    TaskHandle_t source;   // Source task
    bool ownsData;         // Whether this event owns the data
    
    Event(uint32_t id, void* data = nullptr, size_t size = 0, bool copyData = false);
    ~Event();
    
    // Prevent copying to avoid double-free
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    
    // Allow moving
    Event(Event&& other) noexcept;
    Event& operator=(Event&& other) noexcept;
};

class EventBus {
public:
    using EventCallback = std::function<void(const Event&)>;
    
    static EventBus& getInstance();
    
    // Register listener for specific event
    void on(uint32_t eventId, EventCallback callback);
    
    // Register global listener (receives all events)
    void onAny(EventCallback callback);
    
    // Unregister listener
    void off(uint32_t eventId);
    
    // Send event (thread-safe)
    bool send(uint32_t eventId, void* data = nullptr, size_t dataSize = 0, bool copyData = false);
    
    // Broadcast to all listeners
    bool broadcast(uint32_t eventId, void* data = nullptr, size_t dataSize = 0);
    
    // Process pending events (called by dispatcher task)
    void processEvents();
    
    // Statistics
    size_t getQueuedEvents() const;
    size_t getListenerCount(uint32_t eventId) const;
    
private:
    EventBus();
    ~EventBus();
    
    QueueHandle_t eventQueue;
    SemaphoreHandle_t listenersMutex;
    std::map<uint32_t, std::vector<EventCallback>> listeners;
    std::vector<EventCallback> globalListeners;
    
    static constexpr size_t EVENT_QUEUE_SIZE = 50;
    static constexpr TickType_t QUEUE_TIMEOUT = pdMS_TO_TICKS(100);
    
    void dispatchEvent(Event& event);
};

// Compile-time string hashing for event IDs
constexpr uint32_t hash(const char* str, uint32_t hash = 5381) {
    return (*str == 0) ? hash : ESPLooper::hash(str + 1, ((hash << 5) + hash) + (*str));
}

} // namespace ESPLooper

// Convenient macro for event IDs
#define EVENT_ID(name) ESPLooper::hash(name)