#include <ESPLooper.h>

// ===== Test 1: Task Control (enable/disable/toggle) =====
LP_TIMER_("test_timer", 500, []() {
    switch (Looper.thisState()) {
        case tState::Setup:
            Serial.println("[TestTimer] Setup");
            break;
            
        case tState::Loop:
            Serial.println("[TestTimer] Tick");
            break;
            
        case tState::Event:
            Serial.println("[TestTimer] Event received");
            break;
            
        case tState::Exit:
            Serial.println("[TestTimer] Exit");
            break;
    }
});

// ===== Test 2: Task Lookup by ID =====
LP_TIMER(1000, []() {
    // Test operator[]
    auto timer = Looper["test_timer"];
    if (timer) {
        Serial.println("[Lookup] Found test_timer using operator[]");
        
        // Test toggle
        timer->toggle();
        Serial.printf("[Lookup] Toggled test_timer - now %s\n", 
                     timer->isEnabled() ? "enabled" : "disabled");
    }
    
    // Test getTimer
    auto timerTyped = Looper.getTimer("test_timer");
    if (timerTyped) {
        Serial.println("[Lookup] Found test_timer using getTimer()");
        Serial.printf("[Lookup] Timer period: %dms\n", timerTyped->getPeriod());
    }
});

// ===== Test 3: Event-Driven Thread =====
LP_THREAD_("event_worker", {
    switch (Looper.thisState()) {
        case tState::Setup:
            Serial.println("[EventWorker] Setup");
            break;
            
        case tState::Loop:
            // Don't do anything in normal loop
            LP_DELAY(100);
            break;
            
        case tState::Event: {
            Serial.println("[EventWorker] Event received!");
            int* value = (int*)Looper.eventData();
            if (value) {
                Serial.printf("[EventWorker] Processing value: %d\n", *value);
                LP_DELAY(300);
                Serial.printf("[EventWorker] Value doubled: %d\n", *value * 2);
            }
            break;
        }
            
        case tState::Exit:
            Serial.println("[EventWorker] Exit");
            break;
    }
});

// ===== Test 4: Send Events to Tasks =====
LP_TIMER(2000, []() {
    static int counter = 0;
    counter++;
    
    Serial.printf("\n[EventSender] Sending event with value: %d\n", counter);
    
    // Send to thread
    LP_SEND_EVENT("event_worker", &counter);
    
    // Send to timer
    LP_SEND_EVENT("test_timer", &counter);
});

// ===== Test 5: State Query Methods =====
LP_TICKER_("state_monitor", []() {
    static uint32_t loopCount = 0;
    if (++loopCount % 1000000 == 0) {
        Serial.println("\n[StateMonitor] Current state:");
        Serial.printf("  - Task name: %s\n", Looper.thisTaskName());
        Serial.printf("  - State: ");
        
        if (Looper.thisSetup()) Serial.println("Setup");
        else if (Looper.thisLoop()) Serial.println("Loop");
        else if (Looper.thisEvent()) Serial.println("Event");
        else if (Looper.thisExit()) Serial.println("Exit");
        else Serial.println("Unknown");
        
        Serial.println("");
    }
});

// ===== Test 6: Task Type Checking =====
LP_TIMER(5000, []() {
    Serial.println("\n[TypeChecker] Checking task types:");
    
    auto timer = Looper.getTask("test_timer");
    if (timer) {
        Serial.printf("  test_timer - isTimer: %d, isTicker: %d, isThread: %d\n",
                     timer->isTimer(), timer->isTicker(), timer->isThread());
    }
    
    auto worker = Looper.getTask("event_worker");
    if (worker) {
        Serial.printf("  event_worker - isTimer: %d, isTicker: %d, isThread: %d\n",
                     worker->isTimer(), worker->isTicker(), worker->isThread());
    }
    
    auto monitor = Looper.getTask("state_monitor");
    if (monitor) {
        Serial.printf("  state_monitor - isTimer: %d, isTicker: %d, isThread: %d\n",
                     monitor->isTimer(), monitor->isTicker(), monitor->isThread());
    }
});

// ===== Test 7: Event/State Enable/Disable =====
LP_TIMER(10000, []() {
    static bool eventsEnabled = true;
    
    auto worker = Looper.getTask("event_worker");
    if (worker) {
        eventsEnabled = !eventsEnabled;
        
        if (eventsEnabled) {
            worker->enableEvents();
            Serial.println("\n[EventControl] Events ENABLED for event_worker");
        } else {
            worker->disableEvents();
            Serial.println("\n[EventControl] Events DISABLED for event_worker");
        }
    }
});

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("ESP-Looper: Complete API Test");
    Serial.println("========================================\n");
    Serial.println("This example tests ALL features:");
    Serial.println("  1. Task control (enable/disable/toggle)");
    Serial.println("  2. Task lookup by ID");
    Serial.println("  3. Event-driven threads");
    Serial.println("  4. Event dispatch to tasks");
    Serial.println("  5. State query methods");
    Serial.println("  6. Task type checking");
    Serial.println("  7. Event/State enable/disable");
    Serial.println("\n========================================\n");
    
    ESP_LOOPER.begin();
    
    Serial.println("All tasks started!\n");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
