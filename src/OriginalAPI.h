#pragma once

// ESP-Looper: Original Looper API Implementation
// Provides full compatibility with original Looper macros but using FreeRTOS

#include "Looper.h"
#include "AutoTask.h"
#include <freertos/semphr.h>

// Helper macros for stringification and concatenation
#define _LP_STRINGIFY(x) #x
#define _LP_STRINGIFY_EXPAND(x) _LP_STRINGIFY(x)
#define _LP_CONCAT_IMPL(a, b) a##b
#define _LP_CONCAT(a, b) _LP_CONCAT_IMPL(a, b)

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
    
    bool isTicker() const override { return true; }
    
protected:
    void run() override {
        while (shouldRun) {
            if (enabled) {
                if (statesEnabled) {
                    executeWithState(tState::Loop);
                } else if (callback) {
                    callback();
                }
            }
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
        : Task(name, callback, stackSize, priority, coreId), _case(0), _delayUntil(0), _eventFlag(false) {
        start();
    }
    
    bool isThread() const override { return true; }
    
    // Thread state for Duff's Device pattern
    uint16_t _case;
    TickType_t _delayUntil;
    bool _eventFlag;
    
protected:
    void run() override {
        while (shouldRun) {
            if (enabled) {
                if (statesEnabled) {
                    executeWithState(tState::Loop);
                } else if (callback) {
                    callback();
                }
            }
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
        Looper::getInstance().addTicker(name, task);
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
        Looper::getInstance().addThread(name, task);
        threadHandle = task;  // Store handle for access
    }
    
    // Public access to thread handle - required by LP_THREAD macros to access thread state
    std::shared_ptr<ThreadTask> threadHandle;
    
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

// Unnamed ticker - uses __LINE__ for unique variable name
#define LP_TICKER(callback, ...) \
    static ESPLooper::AutoTicker _LP_CONCAT(_lp_ticker_, __LINE__)( \
        "ticker_" _LP_STRINGIFY(__LINE__), callback, ##__VA_ARGS__)

// Named ticker - uses string ID directly
#define LP_TICKER_(id, callback, ...) \
    static ESPLooper::AutoTicker _LP_CONCAT(_lp_ticker_named_, __LINE__)( \
        id, callback, ##__VA_ARGS__)

// ===== TIMER - Periodic task (already defined in Looper.h) =====
// LP_TIMER is already available from Looper.h
// For compatibility, provide LP_TIMER_ (named version)
#ifndef LP_TIMER_
#define LP_TIMER_(id, ms, callback, ...) \
    static ESPLooper::AutoTimer _LP_CONCAT(_lp_timer_named_, __LINE__)( \
        id, ms, callback, true, ##__VA_ARGS__)
#endif

// ===== THREAD - Thread with state machine support =====

// Thread state machine control macros
#define LP_THREAD_BEGIN() \
    switch (_LP_THREAD_HANDLE->_case) { \
        case 0:;

#define LP_THREAD_END() \
    } \
    _LP_THREAD_HANDLE->_case = 0;

// Thread control macros
#define LP_RESTART() \
    do { \
        _LP_THREAD_HANDLE->_case = 0; \
        return; \
    } while (0)

#define LP_EXIT() \
    do { \
        _LP_THREAD_HANDLE->_case = __LINE__; \
        return; \
        case __LINE__:; \
    } while (0)

#define LP_WAIT(cond) \
    do { \
        _LP_THREAD_HANDLE->_case = __LINE__; \
        case __LINE__: \
        if (!(cond)) return; \
    } while (0)

#define LP_DELAY(ms) \
    do { \
        _LP_THREAD_HANDLE->_delayUntil = xTaskGetTickCount() + pdMS_TO_TICKS(ms); \
        LP_WAIT(xTaskGetTickCount() >= _LP_THREAD_HANDLE->_delayUntil); \
    } while (0)

#define LP_WAIT_EVENT() \
    do { \
        _LP_THREAD_HANDLE->_case = __LINE__; \
        case __LINE__: \
        if (!_LP_THREAD_HANDLE->_eventFlag) return; \
        _LP_THREAD_HANDLE->_eventFlag = false; \
    } while (0)

// Helper macro for thread body
#define _LP_THREAD_INNER(body) \
    LP_THREAD_BEGIN(); \
    body \
    LP_THREAD_END();

// Unnamed thread
#define LP_THREAD(body, ...) \
    namespace { \
        struct _LP_CONCAT(_lp_thread_wrapper_, __LINE__) { \
            /* Static member to store thread handle - each instance in anonymous namespace is unique per TU */ \
            static inline std::shared_ptr<ESPLooper::ThreadTask> handle; \
            _LP_CONCAT(_lp_thread_wrapper_, __LINE__)() { \
                static ESPLooper::AutoThread reg( \
                    "thread_" _LP_STRINGIFY(__LINE__), \
                    []() { \
                        auto _lp_thread_handle = _LP_CONCAT(_lp_thread_wrapper_, __LINE__)::handle; \
                        if (!_lp_thread_handle) { \
                            _lp_thread_handle = std::static_pointer_cast<ESPLooper::ThreadTask>( \
                                ESP_LOOPER.getTask("thread_" _LP_STRINGIFY(__LINE__))); \
                            _LP_CONCAT(_lp_thread_wrapper_, __LINE__)::handle = _lp_thread_handle; \
                        } \
                        auto _LP_THREAD_HANDLE = _lp_thread_handle; \
                        _LP_THREAD_INNER(body) \
                    }, \
                    ##__VA_ARGS__); \
                _LP_CONCAT(_lp_thread_wrapper_, __LINE__)::handle = reg.threadHandle; \
            } \
        } _LP_CONCAT(_lp_thread_instance_, __LINE__); \
    }

// Named thread
#define LP_THREAD_(id, body, ...) \
    namespace { \
        struct _LP_CONCAT(_lp_thread_wrapper_named_, __LINE__) { \
            static inline std::shared_ptr<ESPLooper::ThreadTask> handle; \
            _LP_CONCAT(_lp_thread_wrapper_named_, __LINE__)() { \
                static ESPLooper::AutoThread reg( \
                    id, \
                    []() { \
                        auto _lp_thread_handle = _LP_CONCAT(_lp_thread_wrapper_named_, __LINE__)::handle; \
                        if (!_lp_thread_handle) { \
                            _lp_thread_handle = std::static_pointer_cast<ESPLooper::ThreadTask>( \
                                ESP_LOOPER.getTask(id)); \
                            _LP_CONCAT(_lp_thread_wrapper_named_, __LINE__)::handle = _lp_thread_handle; \
                        } \
                        auto _LP_THREAD_HANDLE = _lp_thread_handle; \
                        _LP_THREAD_INNER(body) \
                    }, \
                    ##__VA_ARGS__); \
                _LP_CONCAT(_lp_thread_wrapper_named_, __LINE__)::handle = reg.threadHandle; \
            } \
        } _LP_CONCAT(_lp_thread_instance_named_, __LINE__); \
    }

// ===== LISTENER - Event listener (reuse existing) =====

// Named listener only (listeners always need an event ID)
#define LP_LISTENER_(id, callback, ...) \
    static ESPLooper::AutoListener _LP_CONCAT(_lp_listener_, __LINE__)( \
        "listener_" id, EVENT_ID(id), callback, ##__VA_ARGS__)

// ===== EVENTS =====

// Helper function template for safe event data size calculation
namespace _lp_event_helpers {
    // Template for typed pointers - returns sizeof(*ptr) if non-null
    template<typename T>
    inline size_t safe_sizeof(T* ptr) {
        return ptr ? sizeof(*ptr) : 0;
    }
    
    // Overload for void* - size information is lost, return 0
    inline size_t safe_sizeof(void* ptr) {
        return 0;
    }
    
    // Overload for const void* - size information is lost, return 0
    inline size_t safe_sizeof(const void* ptr) {
        return 0;
    }
    
    // Overload for nullptr_t - always return 0 for null
    inline size_t safe_sizeof(std::nullptr_t) {
        return 0;
    }
}

// Events - handle null data properly
#define LP_SEND_EVENT(id, data) \
    ESP_LOOPER.sendEvent(EVENT_ID(id), static_cast<void*>(data), \
        _lp_event_helpers::safe_sizeof(data), true)

#define LP_PUSH_EVENT(id, data) \
    ESP_LOOPER.sendEvent(EVENT_ID(id), static_cast<void*>(data), \
        _lp_event_helpers::safe_sizeof(data), true)

#define LP_BROADCAST EVENT_ID("")

// ===== SEMAPHORES - Using FreeRTOS semaphores =====
typedef SemaphoreHandle_t LP_SEM;

#define LP_SEM_CREATE() xSemaphoreCreateBinary()
#define LP_SEM_SIGNAL(sem) xSemaphoreGive(sem)
#define LP_SEM_WAIT(sem) \
    LP_WAIT(xSemaphoreTake(sem, 0) == pdTRUE)
