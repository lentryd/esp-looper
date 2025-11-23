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

**Key Difference**: Everything runs as **real FreeRTOS tasks** for true parallelism across both CPU cores!

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