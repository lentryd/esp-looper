#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <string>
#include "Event.h"

// Task execution state (Setup/Loop/Event/Exit) - Global scope for easy access
enum class tState {
    Setup,   // Called once when task starts
    Loop,    // Normal execution
    Event,   // When event is sent to this task's ID
    Exit     // Before task is removed
};

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
    
    // Original Looper task control
    void enable();
    void disable();
    bool isEnabled() const;
    void toggle();
    
    // State
    TaskState getState() const { return state; }
    TaskHandle_t getHandle() const { return taskHandle; }
    const char* getName() const { return taskName.c_str(); }
    BaseType_t getCoreId() const { return coreId; }
    
    // Task info
    uint32_t getId() const { return taskId; }
    const char* getIdString() const { return taskIdString; }
    
    // Task type checks
    virtual bool isTimer() const { return false; }
    virtual bool isTicker() const { return false; }
    virtual bool isThread() const { return false; }
    virtual bool isListener() const { return false; }
    
    // Event handling
    void enableEvents();
    void disableEvents();
    bool hasEvents() const;
    
    // State handling
    void enableStates();
    void disableStates();
    bool hasStates() const;
    
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
    
    // Original Looper state management
    uint32_t taskId;           // Hash ID
    const char* taskIdString;  // String ID
    bool enabled;
    bool eventsEnabled;
    bool statesEnabled;
    tState currentState;
    bool setupCalled;          // Track if Setup has been called
    
    static void taskWrapper(void* parameter);
    virtual void run();
    
    // State execution wrapper
    void executeWithState(tState state);
    
    friend class Looper;
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
    bool isTimer() const override { return true; }
    
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
    bool isListener() const override { return true; }
    
protected:
    uint32_t listenEventId;
    EventCallback eventCallback;
};

} // namespace ESPLooper