#include "Looper.h"
#include "AutoTask.h"
#include "OriginalAPI.h"

namespace ESPLooper {

Looper::Looper() 
    : eventDispatcherHandle(nullptr), initialized(false),
      currentState(tState::Loop), currentEventData(nullptr), currentTask(nullptr) {
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
    
    // Initialize all auto-registered tasks
    AutoTask::initAll();
    
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
    
    // Store ID for lookup
    uint32_t hashId = EVENT_ID(name);
    task->taskId = hashId;
    task->taskIdString = name;
    
    // Enable events and states by default
    task->enableEvents();
    task->enableStates();
    
    // Add to map
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        taskMap[hashId] = task;
        xSemaphoreGive(tasksMutex);
    }
    
    addTask(task);
    
    // Call Setup state
    currentTask = task;
    task->executeWithState(tState::Setup);
    currentTask = nullptr;
    
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
    
    // Store ID for lookup
    uint32_t hashId = EVENT_ID(name);
    task->taskId = hashId;
    task->taskIdString = name;
    
    // Enable events and states by default
    task->enableEvents();
    task->enableStates();
    
    // Add to map
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        taskMap[hashId] = task;
        xSemaphoreGive(tasksMutex);
    }
    
    addTask(task);
    return task;
}

void Looper::addTicker(const char* name, std::shared_ptr<TickerTask> task) {
    // Store ID for lookup
    uint32_t hashId = EVENT_ID(name);
    task->taskId = hashId;
    task->taskIdString = name;
    
    // Enable events and states by default
    task->enableEvents();
    task->enableStates();
    
    // Add to map
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        taskMap[hashId] = task;
        xSemaphoreGive(tasksMutex);
    }
    
    addTask(task);
    
    // Call Setup state
    currentTask = task;
    task->executeWithState(tState::Setup);
    currentTask = nullptr;
}

void Looper::addThread(const char* name, std::shared_ptr<ThreadTask> task) {
    // Store ID for lookup
    uint32_t hashId = EVENT_ID(name);
    task->taskId = hashId;
    task->taskIdString = name;
    
    // Enable events and states by default
    task->enableEvents();
    task->enableStates();
    
    // Add to map
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        taskMap[hashId] = task;
        xSemaphoreGive(tasksMutex);
    }
    
    addTask(task);
    
    // Call Setup state
    currentTask = task;
    task->executeWithState(tState::Setup);
    currentTask = nullptr;
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

// ===== Task Lookup Methods (Original Looper API) =====

std::shared_ptr<Task> Looper::operator[](const char* id) {
    return getTask(EVENT_ID(id));
}

std::shared_ptr<Task> Looper::getTask(uint32_t id) {
    std::shared_ptr<Task> result;
    if (xSemaphoreTake(tasksMutex, portMAX_DELAY)) {
        auto it = taskMap.find(id);
        if (it != taskMap.end()) {
            result = it->second;
        }
        xSemaphoreGive(tasksMutex);
    }
    return result;
}

std::shared_ptr<TimerTask> Looper::getTimer(const char* id) {
    return getTimer(EVENT_ID(id));
}

std::shared_ptr<TimerTask> Looper::getTimer(uint32_t id) {
    return std::dynamic_pointer_cast<TimerTask>(getTask(id));
}

std::shared_ptr<ListenerTask> Looper::getListener(const char* id) {
    return getListener(EVENT_ID(id));
}

std::shared_ptr<ListenerTask> Looper::getListener(uint32_t id) {
    return std::dynamic_pointer_cast<ListenerTask>(getTask(id));
}

std::shared_ptr<TickerTask> Looper::getTicker(const char* id) {
    return getTicker(EVENT_ID(id));
}

std::shared_ptr<TickerTask> Looper::getTicker(uint32_t id) {
    return std::dynamic_pointer_cast<TickerTask>(getTask(id));
}

std::shared_ptr<ThreadTask> Looper::getThread(const char* id) {
    return getThread(EVENT_ID(id));
}

std::shared_ptr<ThreadTask> Looper::getThread(uint32_t id) {
    return std::dynamic_pointer_cast<ThreadTask>(getTask(id));
}

// ===== State Management Methods =====

tState Looper::thisState() const {
    return currentTask ? currentTask->currentState : currentState;
}

bool Looper::thisSetup() const {
    return thisState() == tState::Setup;
}

bool Looper::thisLoop() const {
    return thisState() == tState::Loop;
}

bool Looper::thisEvent() const {
    return thisState() == tState::Event;
}

bool Looper::thisExit() const {
    return thisState() == tState::Exit;
}

void* Looper::eventData() const {
    return currentEventData;
}

const char* Looper::thisTaskName() const {
    return currentTask ? currentTask->getName() : nullptr;
}

void Looper::executeTaskWithEvent(std::shared_ptr<Task> task, const Event& event) {
    if (!task || !task->hasEvents()) {
        return;
    }
    
    currentTask = task;
    currentEventData = event.data;
    
    task->executeWithState(tState::Event);
    
    currentTask = nullptr;
    currentEventData = nullptr;
}

} // namespace ESPLooper