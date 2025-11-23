# ESP-Looper

Multi-threaded event-driven framework for ESP32 with FreeRTOS integration.

## Features

✅ **True Multithreading** - Each task runs in its own FreeRTOS thread  
✅ **Thread-Safe Events** - Send events between tasks on different cores  
✅ **Core Pinning** - Pin tasks to specific CPU cores (0 or 1)  
✅ **Zero-Copy Option** - Choose between data copying or referencing  
✅ **High Performance** - Event queue with configurable size  
✅ **Easy API** - Simple macros and C++ interface  
✅ **Statistics** - Monitor task stack usage and event queue  

## Installation

1. Download or clone this repository
2. Copy to Arduino libraries folder
3. Include in your sketch: `#include <ESPLooper.h>`

## Quick Start

```cpp
#include <ESPLooper.h>

void setup() {
    ESP_LOOPER.begin();
    
    // Timer task - runs every 1000ms
    ESP_TIMER("sensor", 1000, []() {
        int data = analogRead(34);
        ESP_SEND_EVENT(EVENT_ID("data"), &data, sizeof(data));
    });
    
    // Event listener
    ESP_LISTENER("display", EVENT_ID("data"), [](const ESPLooper::Event& evt) {
        Serial.printf("Data: %d\n", *(int*)evt.data);
    });
}
```

## 🎯 Auto-Registration (Like Original Looper)

ESP-Looper supports automatic task registration, similar to the original Looper library. Define tasks globally without calling registration functions:

```cpp
#include <ESPLooper.h>

// Global auto-registered timer - runs automatically!
LP_TIMER(1000, []() {
    int data = analogRead(34);
    LP_SEND_EVENT(EVENT_ID("sensor"), &data, sizeof(data));
});

// Global auto-registered listener
LP_LISTENER(EVENT_ID("sensor"), [](const ESPLooper::Event& evt) {
    Serial.printf("Data: %d\n", *(int*)evt.data);
});

void setup() {
    ESP_LOOPER.begin(); // Automatically initializes all global tasks
}
```

### Auto-Registration Macros

| Macro | Description |
|-------|-------------|
| `LP_TIMER(period, callback, ...)` | Auto-registered timer with auto-generated name |
| `LP_TIMER_NAMED(name, period, callback, ...)` | Auto-registered timer with custom name |
| `LP_LISTENER(eventId, callback, ...)` | Auto-registered listener with auto-generated name |
| `LP_LISTENER_NAMED(name, eventId, callback, ...)` | Auto-registered listener with custom name |
| `LP_SEND_EVENT(eventId, data, size)` | Short event sending macro |

### Benefits

✅ **Cleaner Code** - Define tasks where they logically belong  
✅ **No Setup Clutter** - Keep setup() minimal  
✅ **Familiar Pattern** - Same as original Looper library  
✅ **Mix & Match** - Use auto-registration with dynamic tasks  

## 🎯 Original Looper API

ESP-Looper implements the **full original Looper API**, but with real FreeRTOS multithreading instead of cooperative scheduling:

### LP_TICKER - Continuous Task
Runs continuously in a loop:
```cpp
LP_TICKER([]() {
    // Called continuously (with taskYIELD)
});

LP_TICKER_("my_ticker", []() {
    // Named ticker
});
```

### LP_TIMER - Periodic Task
Executes at regular intervals:
```cpp
LP_TIMER(1000, []() {
    // Called every 1000ms (auto-generated name)
});

LP_TIMER_("timer_1sec", 1000, []() {
    // Called every 1000ms (named)
});
```

### LP_THREAD - Async Thread with State Machine
True async programming with Duff's Device pattern:
```cpp
LP_THREAD({
    while (true) {
        Serial.println("Hello");
        LP_DELAY(1000);  // Non-blocking delay!
        
        LP_WAIT(condition);  // Wait for condition
    }
});

LP_THREAD_("named_thread", {
    // Thread with custom name
});
```

### Thread Control Macros
- **`LP_DELAY(ms)`** - Non-blocking delay using state machine
- **`LP_WAIT(cond)`** - Wait for condition to be true
- **`LP_WAIT_EVENT()`** - Wait for event to arrive
- **`LP_EXIT()`** - Exit thread and resume at this point next time
- **`LP_RESTART()`** - Restart thread from beginning

### Events
```cpp
LP_SEND_EVENT("event", &data);
LP_PUSH_EVENT("event", &data);

LP_LISTENER_("event", [](const ESPLooper::Event& evt) {
    // Handle event
});
```

### Semaphores (FreeRTOS)
```cpp
LP_SEM mySem = LP_SEM_CREATE();

LP_THREAD_("producer", {
    while (true) {
        // Produce data
        LP_SEM_SIGNAL(mySem);
    }
});

LP_THREAD_("consumer", {
    while (true) {
        LP_SEM_WAIT(mySem);
        // Consume data
    }
});
```

### All Original Features Supported
✅ **LP_TICKER** - Continuous tasks  
✅ **LP_TIMER** - Periodic tasks  
✅ **LP_THREAD** - Threads with Duff's Device state machine  
✅ **LP_LISTENER** - Event listeners  
✅ **LP_DELAY, LP_WAIT, LP_EXIT, LP_RESTART** - Thread control  
✅ **LP_SEM** - FreeRTOS semaphores  
✅ **Named versions** - Use `_` suffix macros for custom IDs  
✅ **Auto-registration** - Define globally, start automatically  
✅ **Task Control** - enable(), disable(), toggle() for dynamic control  
✅ **Task Lookup** - Access tasks by ID: `Looper["id"]`  
✅ **State Management** - Setup/Loop/Event/Exit states for all tasks  
✅ **Event-Driven Tasks** - Tasks receive events sent to their ID  

**Key Difference**: Everything runs as **real FreeRTOS tasks** for true parallelism across both CPU cores!

## 🎯 Task Control & State Management

ESP-Looper now supports the complete Original Looper task control API:

### Task Control
Control any task dynamically:
```cpp
LP_TIMER_("my_timer", 1000, []() {
    Serial.println("Tick");
});

// Control the timer
Looper["my_timer"]->disable();  // Stop execution
Looper["my_timer"]->enable();   // Resume execution
Looper["my_timer"]->toggle();   // Toggle on/off
bool running = Looper["my_timer"]->isEnabled();
```

### Task Lookup
Access tasks by ID:
```cpp
auto task = Looper["task_id"];        // Get any task
auto timer = Looper.getTimer("id");   // Get timer specifically
auto thread = Looper.getThread("id"); // Get thread specifically
auto ticker = Looper.getTicker("id"); // Get ticker specifically
```

### State Management
All tasks support Setup/Loop/Event/Exit states:
```cpp
LP_TIMER_("sensor", 100, []() {
    switch (Looper.thisState()) {
        case tState::Setup:
            // Called once when task starts
            pinMode(SENSOR_PIN, INPUT);
            Serial.println("Sensor initialized");
            break;
            
        case tState::Loop:
            // Normal periodic execution
            int value = analogRead(SENSOR_PIN);
            Serial.println(value);
            break;
            
        case tState::Event:
            // When event is sent to "sensor" ID
            Serial.println("Event received!");
            void* data = Looper.eventData();
            break;
            
        case tState::Exit:
            // Before task is removed
            Serial.println("Sensor shutting down");
            break;
    }
});
```

### Event-Driven Threads
Threads can be event-driven:
```cpp
LP_THREAD_("action", {
    // Only execute when event is sent to "action"
    if (Looper.thisState() != tState::Event)
        return;
    
    int* value = (int*)Looper.eventData();
    Serial.printf("Action triggered with value: %d\n", *value);
    
    LP_DELAY(500);
    Serial.println("Action complete!");
});

// Send event to trigger the thread
int data = 42;
LP_SEND_EVENT("action", &data);
```

### State Query Methods
```cpp
Looper.thisState()     // Get current state
Looper.thisSetup()     // Check if in Setup state
Looper.thisLoop()      // Check if in Loop state
Looper.thisEvent()     // Check if in Event state
Looper.thisExit()      // Check if in Exit state
Looper.eventData()     // Get event data pointer
Looper.thisTaskName()  // Get current task name
```

## API Reference

### Initialize Framework
```cpp
ESP_LOOPER.begin();
```

### Create Timer Task
```cpp
ESP_TIMER(name, period_ms, callback, autoStart, coreId);
```

### Create Event Listener
```cpp
ESP_LISTENER(name, eventId, callback, coreId);
```

### Send Event
```cpp
ESP_SEND_EVENT(eventId, data, size);        // Copy data
ESP_SEND_EVENT_REF(eventId, data, size);    // Reference data
```

### Event ID
```cpp
EVENT_ID("my_event")  // Compile-time hash
```

## Architecture

```
┌─────────────────────────────────────────┐
│           ESP-Looper Framework          │
├─────────────────────────────────────────┤
│                                         │
│  ┌────────────┐      ┌──────────────┐  │
│  │  Task 1    │──┐   │   EventBus   │  │
│  │  (Core 0)  │  │   │              │  │
│  └────────────┘  │   │  ┌────────┐  │  │
│                  ├──▶│  │ Queue  │  │  │
│  ┌────────────┐  │   │  └────────┘  │  │
│  │  Task 2    │──┘   │              │  │
│  │  (Core 1)  │      │  Dispatcher  │  │
│  └────────────┘  ┌───│              │  │
│                  │   └──────────────┘  │
│  ┌────────────┐  │                     │
│  │ Listener   │◀─┘                     │
│  │  (Core 1)  │                        │
│  └────────────┘                        │
└─────────────────────────────────────────┘
```

## Thread Safety

All operations are thread-safe:
- ✅ Send events from any task
- ✅ Add/remove tasks dynamically
- ✅ Access event bus from interrupts (with care)

## Performance

- Event dispatch: ~50-100μs
- Queue capacity: 50 events (configurable)
- Memory per task: ~100 bytes + stack size
- Supports 100+ concurrent tasks

## Examples

See `examples/` folder:
- `basic` - Simple timer and listener
- `multicore` - Multi-core communication
- `communication` - Multiple producers/consumers
- `auto_registration` - Global task auto-registration
- `original_api` - Full Original Looper API demonstration
- `task_control` - Task enable/disable/toggle with state management
- `event_thread` - Event-driven threads and state callbacks

## Comparison with Original Looper

| Feature | Original Looper | ESP-Looper |
|---------|----------------|------------|
| Threading | Cooperative | Preemptive |
| Cores | Single | Dual-core |
| Events | Sequential | Concurrent |
| Safety | Single-threaded | Thread-safe |
| Performance | Good | Excellent |

## License

MIT License - free for commercial and personal use

## Author

Created by lentryd for ESP32 development