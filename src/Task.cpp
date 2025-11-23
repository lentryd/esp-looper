#include "Task.h"
#include "Looper.h"

namespace ESPLooper {

// ===== Task Implementation =====

Task::Task(const char* name, TaskCallback callback, uint32_t stackSize, 
           UBaseType_t priority, BaseType_t coreId)
    : taskName(name), callback(callback), taskHandle(nullptr), 
      state(TaskState::Created), stackSize(stackSize), 
      priority(priority), coreId(coreId), shouldRun(true),
      taskId(0), taskIdString(nullptr), enabled(true), 
      eventsEnabled(false), statesEnabled(false), currentState(tState::Loop) {
}

Task::~Task() {
    stop();
}

bool Task::start() {
    if (state == TaskState::Running) {
        return false;
    }
    
    shouldRun = true;
    BaseType_t result;
    
    if (coreId == tskNO_AFFINITY) {
        result = xTaskCreate(taskWrapper, taskName.c_str(), stackSize, 
                            this, priority, &taskHandle);
    } else {
        result = xTaskCreatePinnedToCore(taskWrapper, taskName.c_str(), stackSize,
                                         this, priority, &taskHandle, coreId);
    }
    
    if (result == pdPASS) {
        state = TaskState::Running;
        return true;
    }
    
    return false;
}

bool Task::stop() {
    if (state == TaskState::Stopped || !taskHandle) {
        return false;
    }
    
    shouldRun = false;
    state = TaskState::Stopped;
    
    // Delete the task
    if (taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    
    return true;
}

bool Task::pause() {
    if (state != TaskState::Running || !taskHandle) {
        return false;
    }
    
    vTaskSuspend(taskHandle);
    state = TaskState::Paused;
    return true;
}

bool Task::resume() {
    if (state != TaskState::Paused || !taskHandle) {
        return false;
    }
    
    vTaskResume(taskHandle);
    state = TaskState::Running;
    return true;
}

uint32_t Task::getStackHighWaterMark() const {
    if (taskHandle) {
        return uxTaskGetStackHighWaterMark(taskHandle);
    }
    return 0;
}

// ===== Original Looper Task Control =====

void Task::enable() {
    enabled = true;
}

void Task::disable() {
    enabled = false;
}

bool Task::isEnabled() const {
    return enabled;
}

void Task::toggle() {
    enabled = !enabled;
}

void Task::enableEvents() {
    eventsEnabled = true;
}

void Task::disableEvents() {
    eventsEnabled = false;
}

bool Task::hasEvents() const {
    return eventsEnabled;
}

void Task::enableStates() {
    statesEnabled = true;
}

void Task::disableStates() {
    statesEnabled = false;
}

bool Task::hasStates() const {
    return statesEnabled;
}

void Task::executeWithState(tState newState) {
    if (!statesEnabled) {
        if (callback) callback();
        return;
    }
    
    // For Event state, save and restore after callback
    if (newState == tState::Event) {
        tState prevState = currentState;
        currentState = newState;
        if (callback) callback();
        currentState = prevState;
        return;
    }
    
    // For other states, change and keep the state
    currentState = newState;
    
    // Always execute for Exit and Setup states, even if disabled
    // Only check enabled flag for Loop state
    if (callback) {
        if (newState == tState::Exit || newState == tState::Setup) {
            callback();
        } else if (enabled) {
            callback();
        }
    }
}

void Task::taskWrapper(void* parameter) {
    Task* task = static_cast<Task*>(parameter);
    if (task) {
        task->run();
    }
    vTaskDelete(nullptr);
}

void Task::run() {
    while (shouldRun) {
        if (callback) {
            callback();
        }
        taskYIELD();
    }
}

// ===== TimerTask Implementation =====

TimerTask::TimerTask(const char* name, TaskCallback callback, uint32_t periodMs,
                     bool autoStart, uint32_t stackSize, UBaseType_t priority,
                     BaseType_t coreId)
    : Task(name, callback, stackSize, priority, coreId),
      periodMs(periodMs), autoStart(autoStart) {
    
    if (autoStart) {
        start();
    }
}

void TimerTask::setPeriod(uint32_t ms) {
    periodMs = ms;
}

bool TimerTask::start() {
    if (!autoStart && state == TaskState::Created) {
        autoStart = true;
    }
    return Task::start();
}

void TimerTask::run() {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(periodMs);
    
    while (shouldRun) {
        if (enabled) {
            if (statesEnabled) {
                executeWithState(tState::Loop);
            } else if (callback) {
                callback();
            }
        }
        vTaskDelayUntil(&lastWakeTime, period);
    }
}

// ===== ListenerTask Implementation =====

ListenerTask::ListenerTask(const char* name, uint32_t eventId, EventCallback callback,
                           uint32_t stackSize, UBaseType_t priority, BaseType_t coreId)
    : Task(name, nullptr, stackSize, priority, coreId),
      listenEventId(eventId), eventCallback(callback) {
    
    // Register with EventBus
    EventBus::getInstance().on(eventId, [this](const Event& evt) {
        if (eventCallback) {
            eventCallback(evt);
        }
    });
    
    // ListenerTask doesn't need its own execution loop
    // Events are dispatched by the EventBus
}

ListenerTask::~ListenerTask() {
    // Note: We don't unregister from EventBus here to avoid issues
    // if multiple listeners use the same event ID
}

} // namespace ESPLooper