#pragma once

// ESP-Looper: Original Looper API Implementation
// Provides full compatibility with original Looper macros but using FreeRTOS

#include "Looper.h"
#include "AutoTask.h"
#include <freertos/semphr.h>

namespace ESPLooper {

// ===== Ticker Task - Continuously running task =====
class TickerTask : public Task {
public:
    TickerTask(const char* name, TaskCallback callback, 
               uint32_t stackSize = 4096,
               UBaseType_t priority = 1,
               BaseType_t coreId = tskNO_AFFINITY)
        : Task(name, callback, stackSize, priority, coreId) {
        start();
    }
    
protected:
    void run() override {
        while (shouldRun) {
            if (callback) callback();
            taskYIELD();  // Let other tasks run
        }
    }
};

// ===== Thread Task with State Machine (Duff's Device) =====
class ThreadTask : public Task {
public:
    ThreadTask(const char* name, TaskCallback callback,
               uint32_t stackSize = 8192,
               UBaseType_t priority = 1,
               BaseType_t coreId = tskNO_AFFINITY)
        : Task(name, callback, stackSize, priority, coreId), _case(0) {
        start();
    }
    
    // Thread state for Duff's Device pattern
    uint16_t _case;
    TickType_t _delayUntil;
    bool _eventFlag;
    
protected:
    void run() override {
        while (shouldRun) {
            if (callback) callback();
            taskYIELD();
        }
    }
};

// ===== Auto Ticker - Auto-registered ticker task =====
class AutoTicker : public AutoTask {
public:
    AutoTicker(const char* name, Task::TaskCallback callback,
               uint32_t stackSize = 4096,
               UBaseType_t priority = 1,
               BaseType_t coreId = tskNO_AFFINITY)
        : name(name), callback(callback), stackSize(stackSize), 
          priority(priority), coreId(coreId) {}
    
    void init() override {
        auto task = std::make_shared<TickerTask>(name, callback, stackSize, priority, coreId);
        Looper::getInstance().addTask(task);
    }
    
private:
    const char* name;
    Task::TaskCallback callback;
    uint32_t stackSize;
    UBaseType_t priority;
    BaseType_t coreId;
};

// ===== Auto Thread - Auto-registered thread task =====
class AutoThread : public AutoTask {
public:
    AutoThread(const char* name, Task::TaskCallback callback,
               uint32_t stackSize = 8192,
               UBaseType_t priority = 1,
               BaseType_t coreId = tskNO_AFFINITY)
        : name(name), callback(callback), stackSize(stackSize),
          priority(priority), coreId(coreId) {}
    
    void init() override {
        auto task = std::make_shared<ThreadTask>(name, callback, stackSize, priority, coreId);
        Looper::getInstance().addTask(task);
    }
    
private:
    const char* name;
    Task::TaskCallback callback;
    uint32_t stackSize;
    UBaseType_t priority;
    BaseType_t coreId;
};

} // namespace ESPLooper

// ========== Original Looper Macros ==========

// ===== TICKER - Continuously running task =====
#define LP_TICKER(callback, ...) \
    static ESPLooper::AutoTicker _lp_ticker_##__LINE__( \
        "ticker_" #__LINE__, callback, ##__VA_ARGS__)

#define LP_TICKER_(id, callback, ...) \
    static ESPLooper::AutoTicker _lp_ticker_##id( \
        #id, callback, ##__VA_ARGS__)

// ===== TIMER - Periodic task (reuse existing) =====
#define LP_TIMER_(id, ms, callback, ...) \
    static ESPLooper::AutoTimer _lp_timer_##id( \
        #id, ms, callback, ##__VA_ARGS__)

// ===== THREAD - Thread with state machine support =====

// Helper to get thread handle - must be unique per line/id
#define _LP_THREAD_HANDLE_NAME_INNER(line) _lp_thread_handle_##line
#define _LP_THREAD_HANDLE_NAME(line) _LP_THREAD_HANDLE_NAME_INNER(line)
#define _LP_THREAD_HANDLE _LP_THREAD_HANDLE_NAME(__LINE__)

#define _LP_THREAD_CASE (_LP_THREAD_HANDLE->_case)

// Thread state machine control
#define LP_THREAD_BEGIN() \
    switch (_LP_THREAD_CASE) { \
        case 0:;

#define LP_THREAD_END() \
    } \
    _LP_THREAD_CASE = 0;

#define _LP_THREAD_INNER(body) \
    LP_THREAD_BEGIN(); \
    body \
    LP_THREAD_END();

// Counter for unique case labels
#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif

// Thread control macros
#define LP_RESTART() \
    do { \
        _LP_THREAD_CASE = 0; \
        return; \
    } while (0)

#define LP_EXIT() \
    do { \
        _LP_THREAD_CASE = __COUNTER__ + 1; \
        return; \
        case __COUNTER__:; \
    } while (0)

#define LP_WAIT(cond) \
    do { \
        _LP_THREAD_CASE = __COUNTER__ + 1; \
        case __COUNTER__: \
            if (!(cond)) return; \
    } while (0)

#define LP_DELAY(ms) \
    do { \
        _LP_THREAD_HANDLE->_delayUntil = xTaskGetTickCount() + pdMS_TO_TICKS(ms); \
        LP_WAIT(xTaskGetTickCount() >= _LP_THREAD_HANDLE->_delayUntil); \
    } while (0)

#define LP_WAIT_EVENT() \
    do { \
        _LP_THREAD_CASE = __COUNTER__ + 1; \
        case __COUNTER__: \
            if (!_LP_THREAD_HANDLE->_eventFlag) return; \
            _LP_THREAD_HANDLE->_eventFlag = false; \
    } while (0)

// Thread macro with auto-generated name
#define LP_THREAD(body, ...) \
    static std::shared_ptr<ESPLooper::ThreadTask> _LP_THREAD_HANDLE; \
    static ESPLooper::AutoThread _lp_thread_obj_##__LINE__( \
        "thread_" #__LINE__, \
        []() { \
            _LP_THREAD_INNER(body) \
        }, \
        ##__VA_ARGS__); \
    namespace { \
        struct _lp_thread_init_##__LINE__ { \
            _lp_thread_init_##__LINE__() { \
                static bool initialized = false; \
                if (!initialized) { \
                    auto& looper = ESPLooper::Looper::getInstance(); \
                    auto getHandle = [&]() { \
                        vTaskDelay(pdMS_TO_TICKS(50)); \
                        _LP_THREAD_HANDLE = std::static_pointer_cast<ESPLooper::ThreadTask>( \
                            looper.getTask("thread_" #__LINE__)); \
                    }; \
                    getHandle(); \
                    initialized = true; \
                } \
            } \
        } _lp_thread_init_instance_##__LINE__; \
    }

// Thread macro with custom name
#define LP_THREAD_(id, body, ...) \
    static std::shared_ptr<ESPLooper::ThreadTask> _lp_thread_handle_##id; \
    static ESPLooper::AutoThread _lp_thread_obj_##id( \
        #id, \
        []() { \
            _LP_THREAD_INNER(body) \
        }, \
        ##__VA_ARGS__); \
    namespace { \
        struct _lp_thread_init_##id { \
            _lp_thread_init_##id() { \
                static bool initialized = false; \
                if (!initialized) { \
                    auto& looper = ESPLooper::Looper::getInstance(); \
                    auto getHandle = [&]() { \
                        vTaskDelay(pdMS_TO_TICKS(50)); \
                        _lp_thread_handle_##id = std::static_pointer_cast<ESPLooper::ThreadTask>( \
                            looper.getTask(#id)); \
                    }; \
                    getHandle(); \
                    initialized = true; \
                } \
            } \
        } _lp_thread_init_instance_##id; \
    }

// ===== LISTENER - Event listener (reuse existing) =====
#define LP_LISTENER_(id, callback, ...) \
    static ESPLooper::AutoListener _lp_listener_##__LINE__( \
        "listener_" #__LINE__, EVENT_ID(id), callback, ##__VA_ARGS__)

// ===== EVENTS =====
#define LP_SEND_EVENT(id, data) \
    ESP_LOOPER.sendEvent(EVENT_ID(id), data, sizeof(*(data)), true)

#define LP_PUSH_EVENT(id, data) \
    ESP_LOOPER.sendEvent(EVENT_ID(id), data, sizeof(*(data)), true)

#define LP_BROADCAST EVENT_ID("")

// ===== SEMAPHORES - Using FreeRTOS semaphores =====
typedef SemaphoreHandle_t LP_SEM;

#define LP_SEM_CREATE() xSemaphoreCreateBinary()
#define LP_SEM_SIGNAL(sem) xSemaphoreGive(sem)
#define LP_SEM_WAIT(sem) \
    LP_WAIT(xSemaphoreTake(sem, 0) == pdTRUE)
