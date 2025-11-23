#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <string>
#include "Event.h"

namespace ESPLooper {

enum class TaskState {
    Created,
    Running,
    Paused,
    Stopped
};

class Task {
public:
    using TaskCallback = std::function<void()>;
    
    Task(const char* name, 
         TaskCallback callback,
         uint32_t stackSize = 4096,
         UBaseType_t priority = 1,
         BaseType_t coreId = tskNO_AFFINITY);
    
    virtual ~Task();
    
    // Task control
    virtual bool start();
    virtual bool stop();
    virtual bool pause();
    virtual bool resume();
    
    // State
    TaskState getState() const { return state; }
    TaskHandle_t getHandle() const { return taskHandle; }
    const char* getName() const { return taskName.c_str(); }
    BaseType_t getCoreId() const { return coreId; }
    
    // Statistics
    uint32_t getStackHighWaterMark() const;
    
protected:
    std::string taskName;
    TaskCallback callback;
    TaskHandle_t taskHandle;
    TaskState state;
    uint32_t stackSize;
    UBaseType_t priority;
    BaseType_t coreId;
    volatile bool shouldRun;
    
    static void taskWrapper(void* parameter);
    virtual void run();
};

// Timer-based periodic task
class TimerTask : public Task {
public:
    TimerTask(const char* name,
              TaskCallback callback,
              uint32_t periodMs,
              bool autoStart = true,
              uint32_t stackSize = 4096,
              UBaseType_t priority = 1,
              BaseType_t coreId = tskNO_AFFINITY);
    
    ~TimerTask() override = default;
    
    void setPeriod(uint32_t ms);
    uint32_t getPeriod() const { return periodMs; }
    
    bool start() override;
    
protected:
    void run() override;
    uint32_t periodMs;
    bool autoStart;
};

// Event listener task
class ListenerTask : public Task {
public:
    using EventCallback = std::function<void(const Event&)>;
    
    ListenerTask(const char* name,
                 uint32_t eventId,
                 EventCallback callback,
                 uint32_t stackSize = 4096,
                 UBaseType_t priority = 1,
                 BaseType_t coreId = tskNO_AFFINITY);
    
    ~ListenerTask() override;
    
    uint32_t getEventId() const { return listenEventId; }
    
protected:
    uint32_t listenEventId;
    EventCallback eventCallback;
};

} // namespace ESPLooper