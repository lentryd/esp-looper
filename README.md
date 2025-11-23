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