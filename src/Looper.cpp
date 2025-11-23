#include "Looper.h"

namespace ESPLooper {

Looper::Looper() 
    : eventDispatcherHandle(nullptr), initialized(false) {
    tasksMutex = xSemaphoreCreateMutex();
}

Looper::~Looper() {
    if (eventDispatcherHandle) {
        vTaskDelete(eventDispatcherHandle);
    }
    if (tasksMutex) {
        vSemaphoreDelete(tasksMutex);
    }
}

Looper& Looper::getInstance() {
    static Looper instance;
    return instance;
}

void Looper::begin(UBaseType_t dispatcherPriority, BaseType_t dispatcherCore) {
    if (initialized) {
        return;
    }
    
    // Create event dispatcher task
    if (dispatcherCore == tskNO_AFFINITY) {
        xTaskCreate(eventDispatcherTask, "EventDispatcher", 4096,
                   this, dispatcherPriority, &eventDispatcherHandle);
    } else {
        xTaskCreatePinnedToCore(eventDispatcherTask, "EventDispatcher", 4096,
                               this, dispatcherPriority, &eventDispatcherHandle,
                               dispatcherCore);
    }
    
    initialized = true;
}

void Looper::addTask(std::shared_ptr<Task> task) {
    if (!task) {
        return;
    }
    
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        tasks.push_back(task);
        xSemaphoreGive(tasksMutex);
    }
}

void Looper::removeTask(std::shared_ptr<Task> task) {
    if (!task) {
        return;
    }
    
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        tasks.erase(
            std::remove(tasks.begin(), tasks.end(), task),
            tasks.end()
        );
        xSemaphoreGive(tasksMutex);
    }
    
    task->stop();
}

void Looper::removeTask(const char* name) {
    auto task = getTask(name);
    if (task) {
        removeTask(task);
    }
}

std::shared_ptr<TimerTask> Looper::addTimer(
    const char* name,
    Task::TaskCallback callback,
    uint32_t periodMs,
    bool autoStart,
    BaseType_t coreId,
    uint32_t stackSize,
    UBaseType_t priority
) {
    auto task = std::make_shared<TimerTask>(
        name, callback, periodMs, autoStart, stackSize, priority, coreId
    );
    addTask(task);
    return task;
}

std::shared_ptr<ListenerTask> Looper::addListener(
    const char* name,
    uint32_t eventId,
    ListenerTask::EventCallback callback,
    BaseType_t coreId,
    uint32_t stackSize,
    UBaseType_t priority
) {
    auto task = std::make_shared<ListenerTask>(
        name, eventId, callback, stackSize, priority, coreId
    );
    addTask(task);
    return task;
}

bool Looper::sendEvent(uint32_t eventId, void* data, size_t dataSize, bool copyData) {
    return EventBus::getInstance().send(eventId, data, dataSize, copyData);
}

bool Looper::sendEvent(const char* eventName, void* data, size_t dataSize, bool copyData) {
    return sendEvent(EVENT_ID(eventName), data, dataSize, copyData);
}

std::shared_ptr<Task> Looper::getTask(const char* name) {
    std::shared_ptr<Task> result;
    
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        for (auto& task : tasks) {
            if (strcmp(task->getName(), name) == 0) {
                result = task;
                break;
            }
        }
        xSemaphoreGive(tasksMutex);
    }
    
    return result;
}

size_t Looper::getTaskCount() const {
    size_t count = 0;
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        count = tasks.size();
        xSemaphoreGive(tasksMutex);
    }
    return count;
}

void Looper::printStats() const {
    Serial.println("=== ESP-Looper Statistics ===");
    Serial.printf("Tasks: %d\n", getTaskCount());
    Serial.printf("Queued Events: %d\n", EventBus::getInstance().getQueuedEvents());
    Serial.println("\nTasks:");
    
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        for (const auto& task : tasks) {
            Serial.printf("  - %s [Core: %d, Stack: %d bytes free]\n",
                         task->getName(),
                         task->getCoreId(),
                         task->getStackHighWaterMark());
        }
        xSemaphoreGive(tasksMutex);
    }
}

void Looper::eventDispatcherTask(void* parameter) {
    EventBus& eventBus = EventBus::getInstance();
    
    while (true) {
        eventBus.processEvents();
        vTaskDelay(pdMS_TO_TICKS(1)); // 1ms delay
    }
}

} // namespace ESPLooper