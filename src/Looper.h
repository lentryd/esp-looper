#pragma once
#include "Event.h"
#include "Task.h"
#include <map>
#include <memory>
#include <vector>

namespace ESPLooper {

// Forward declarations
class TickerTask;
class ThreadTask;

class Looper {
public:
  static Looper &getInstance();

  // Initialize the framework
  void begin(UBaseType_t dispatcherPriority = 3, BaseType_t dispatcherCore = 1);

  // Add tasks
  void addTask(std::shared_ptr<Task> task);
  void removeTask(std::shared_ptr<Task> task);
  void removeTask(const char *name);

  // Create timer task
  std::shared_ptr<TimerTask>
  addTimer(const char *name, Task::TaskCallback callback, uint32_t periodMs,
           bool autoStart = true, BaseType_t coreId = tskNO_AFFINITY,
           uint32_t stackSize = 4096, UBaseType_t priority = 1);

  // Create event listener
  std::shared_ptr<ListenerTask>
  addListener(const char *name, uint32_t eventId,
              ListenerTask::EventCallback callback,
              BaseType_t coreId = tskNO_AFFINITY, uint32_t stackSize = 4096,
              UBaseType_t priority = 1);

  // Add ticker/thread with ID registration (for auto-registration)
  void addTicker(const char *name, std::shared_ptr<TickerTask> task);
  void addThread(const char *name, std::shared_ptr<ThreadTask> task);

  // Event system access
  EventBus &events() { return EventBus::getInstance(); }

  // Send event
  bool sendEvent(uint32_t eventId, void *data = nullptr, size_t dataSize = 0,
                 bool copyData = false);
  bool sendEvent(const char *eventName, void *data = nullptr,
                 size_t dataSize = 0, bool copyData = false);

  // Task management
  std::shared_ptr<Task> getTask(const char *name);
  size_t getTaskCount() const;

  // Task lookup by ID (Original Looper API)
  std::shared_ptr<Task> operator[](const char *id);
  std::shared_ptr<Task> getTask(uint32_t id);

  std::shared_ptr<TimerTask> getTimer(const char *id);
  std::shared_ptr<TimerTask> getTimer(uint32_t id);

  std::shared_ptr<ListenerTask> getListener(const char *id);
  std::shared_ptr<ListenerTask> getListener(uint32_t id);

  std::shared_ptr<TickerTask> getTicker(const char *id);
  std::shared_ptr<TickerTask> getTicker(uint32_t id);

  std::shared_ptr<ThreadTask> getThread(const char *id);
  std::shared_ptr<ThreadTask> getThread(uint32_t id);

  // Current task state info (Original Looper API)
  tState thisState() const;
  bool thisSetup() const;
  bool thisLoop() const;
  bool thisEvent() const;
  bool thisExit() const;

  void *eventData() const;
  const char *thisTaskName() const;

  // Execute task with event
  void executeTaskWithEvent(std::shared_ptr<Task> task, const Event &event);

  // Statistics
  void printStats() const;

  // Current execution context (public for Task access)
  tState currentState;
  void *currentEventData;
  std::shared_ptr<Task> currentTask;

private:
  Looper();
  ~Looper();

  std::vector<std::shared_ptr<Task>> tasks;
  SemaphoreHandle_t tasksMutex;
  TaskHandle_t eventDispatcherHandle;
  bool initialized;

  // Map for fast ID lookup
  std::map<uint32_t, std::shared_ptr<Task>> taskMap;

  friend class Task;

  static void eventDispatcherTask(void *parameter);
};

} // namespace ESPLooper

// Global convenience macros
#define ESP_LOOPER ESPLooper::Looper::getInstance()

#define ESP_TIMER(name, period, callback, ...)                                 \
  ESP_LOOPER.addTimer(name, callback, period, ##__VA_ARGS__)

#define ESP_LISTENER(name, eventId, callback, ...)                             \
  ESP_LOOPER.addListener(name, eventId, callback, ##__VA_ARGS__)

#define ESP_SEND_EVENT(eventId, data, size)                                    \
  ESP_LOOPER.sendEvent(eventId, data, size, true)

#define ESP_SEND_EVENT_REF(eventId, data, size)                                \
  ESP_LOOPER.sendEvent(eventId, data, size, false)

// ========== Auto-registration macros (like original Looper) ==========

// Helper macros for stringification
#define _LP_STRINGIFY(x) #x
#define _LP_STRINGIFY_EXPAND(x) _LP_STRINGIFY(x)

// Auto-registered timer with auto-generated name
#define LP_TIMER(period, callback, ...)                                        \
  static ESPLooper::AutoTimer _lp_timer_##__LINE__(                            \
      "timer_" _LP_STRINGIFY_EXPAND(__LINE__), period, callback,               \
      ##__VA_ARGS__)

// Auto-registered timer with custom name
#define LP_TIMER_NAMED(name, period, callback, ...)                            \
  static ESPLooper::AutoTimer _lp_timer_##name(#name, period, callback,        \
                                               ##__VA_ARGS__)

// Auto-registered listener with auto-generated name
#define LP_LISTENER(eventId, callback, ...)                                    \
  static ESPLooper::AutoListener _lp_listener_##__LINE__(                      \
      "listener_" _LP_STRINGIFY_EXPAND(__LINE__), eventId, callback,           \
      ##__VA_ARGS__)

// Auto-registered listener with custom name
#define LP_LISTENER_NAMED(name, eventId, callback, ...)                        \
  static ESPLooper::AutoListener _lp_listener_##name(#name, eventId, callback, \
                                                     ##__VA_ARGS__)
