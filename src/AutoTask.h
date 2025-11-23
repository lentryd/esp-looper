#pragma once
#include "Looper.h"
#include <vector>

namespace ESPLooper {

// Base class for automatically registered tasks
class AutoTask {
public:
    AutoTask() {
        getRegistry().push_back(this);
    }
    
    virtual ~AutoTask() = default;
    
    // Initialize this task (called automatically by Looper::begin())
    virtual void init() = 0;
    
    // Static registry of all automatic tasks
    static std::vector<AutoTask*>& getRegistry() {
        static std::vector<AutoTask*> registry;
        return registry;
    }
    
    // Initialize all registered tasks
    static void initAll() {
        for (auto* task : getRegistry()) {
            task->init();
        }
    }
};

// Auto-registered timer task
class AutoTimer : public AutoTask {
public:
    AutoTimer(const char* name, uint32_t period, Task::TaskCallback callback, 
              bool autoStart = true, BaseType_t coreId = tskNO_AFFINITY,
              uint32_t stackSize = 4096, UBaseType_t priority = 1)
        : name(name), period(period), callback(callback), 
          autoStart(autoStart), coreId(coreId), 
          stackSize(stackSize), priority(priority) {}
    
    void init() override {
        Looper::getInstance().addTimer(name, callback, period, 
                                      autoStart, coreId, stackSize, priority);
    }
    
private:
    const char* name;
    uint32_t period;
    Task::TaskCallback callback;
    bool autoStart;
    BaseType_t coreId;
    uint32_t stackSize;
    UBaseType_t priority;
};

// Auto-registered listener task
class AutoListener : public AutoTask {
public:
    AutoListener(const char* name, uint32_t eventId, 
                ListenerTask::EventCallback callback,
                BaseType_t coreId = tskNO_AFFINITY,
                uint32_t stackSize = 4096, UBaseType_t priority = 1)
        : name(name), eventId(eventId), callback(callback),
          coreId(coreId), stackSize(stackSize), priority(priority) {}
    
    void init() override {
        Looper::getInstance().addListener(name, eventId, callback, 
                                         coreId, stackSize, priority);
    }
    
private:
    const char* name;
    uint32_t eventId;
    ListenerTask::EventCallback callback;
    BaseType_t coreId;
    uint32_t stackSize;
    UBaseType_t priority;
};

} // namespace ESPLooper
