#include "Event.h"
#include <string.h>

namespace ESPLooper {

// Forward declarations to avoid circular dependency
class Task;
class Looper;

// ===== Event Implementation =====

Event::Event(uint32_t id, void* data, size_t size, bool copyData)
    : id(id), data(data), dataSize(size), source(xTaskGetCurrentTaskHandle()), ownsData(copyData) {
    
    if (copyData && data && size > 0) {
        this->data = malloc(size);
        if (this->data) {
            memcpy(this->data, data, size);
        } else {
            this->ownsData = false;
            this->data = nullptr;
        }
    }
}

Event::~Event() {
    if (ownsData && data) {
        free(data);
        data = nullptr;
    }
}

Event::Event(Event&& other) noexcept
    : id(other.id), data(other.data), dataSize(other.dataSize), 
      source(other.source), ownsData(other.ownsData) {
    other.data = nullptr;
    other.ownsData = false;
}

Event& Event::operator=(Event&& other) noexcept {
    if (this != &other) {
        if (ownsData && data) {
            free(data);
        }
        
        id = other.id;
        data = other.data;
        dataSize = other.dataSize;
        source = other.source;
        ownsData = other.ownsData;
        
        other.data = nullptr;
        other.ownsData = false;
    }
    return *this;
}

// ===== EventBus Implementation =====

EventBus::EventBus() {
    eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(Event*));
    listenersMutex = xSemaphoreCreateMutex();
    
    if (!eventQueue || !listenersMutex) {
        // Fatal error - can't create event system
        abort();
    }
}

EventBus::~EventBus() {
    if (eventQueue) {
        vQueueDelete(eventQueue);
    }
    if (listenersMutex) {
        vSemaphoreDelete(listenersMutex);
    }
}

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

void EventBus::on(uint32_t eventId, EventCallback callback) {
    if (xSemaphoreTake(listenersMutex, portMAX_DELAY)) {
        listeners[eventId].push_back(callback);
        xSemaphoreGive(listenersMutex);
    }
}

void EventBus::onAny(EventCallback callback) {
    if (xSemaphoreTake(listenersMutex, portMAX_DELAY)) {
        globalListeners.push_back(callback);
        xSemaphoreGive(listenersMutex);
    }
}

void EventBus::off(uint32_t eventId) {
    if (xSemaphoreTake(listenersMutex, portMAX_DELAY)) {
        listeners.erase(eventId);
        xSemaphoreGive(listenersMutex);
    }
}

bool EventBus::send(uint32_t eventId, void* data, size_t dataSize, bool copyData) {
    Event* event = new Event(eventId, data, dataSize, copyData);
    if (!event) {
        return false;
    }
    
    if (xQueueSend(eventQueue, &event, QUEUE_TIMEOUT) != pdTRUE) {
        delete event;
        return false;
    }
    
    return true;
}

bool EventBus::broadcast(uint32_t eventId, void* data, size_t dataSize) {
    return send(eventId, data, dataSize, true);
}

void EventBus::processEvents() {
    Event* event = nullptr;
    
    // Process all pending events
    while (xQueueReceive(eventQueue, &event, 0) == pdTRUE) {
        if (event) {
            dispatchEvent(*event);
            delete event;
        }
    }
}

void EventBus::dispatchEvent(Event& event) {
    // Forward declare getInstance to avoid including Looper.h
    extern Looper& getLooperInstance();
    
    if (xSemaphoreTake(listenersMutex, portMAX_DELAY)) {
        // First, send to specific task if event ID matches a task
        auto& looper = getLooperInstance();
        auto task = looper.getTask(event.id);
        if (task && task->hasEvents()) {
            looper.executeTaskWithEvent(task, event);
        }
        
        // Call specific listeners
        auto it = listeners.find(event.id);
        if (it != listeners.end()) {
            for (auto& callback : it->second) {
                callback(event);
            }
        }
        
        // Call global listeners
        for (auto& callback : globalListeners) {
            callback(event);
        }
        
        xSemaphoreGive(listenersMutex);
    }
}

size_t EventBus::getQueuedEvents() const {
    return uxQueueMessagesWaiting(eventQueue);
}

size_t EventBus::getListenerCount(uint32_t eventId) const {
    size_t count = 0;
    if (xSemaphoreTake(listenersMutex, portMAX_DELAY)) {
        auto it = listeners.find(eventId);
        if (it != listeners.end()) {
            count = it->second.size();
        }
        xSemaphoreGive(listenersMutex);
    }
    return count;
}

// Helper function to get Looper instance without including Looper.h
Looper& getLooperInstance() {
    return Looper::getInstance();
}

} // namespace ESPLooper