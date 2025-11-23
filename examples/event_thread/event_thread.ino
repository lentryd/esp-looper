#include <ESPLooper.h>

// Event-driven thread - only runs when event is received
LP_THREAD_("action", {
    // Check if this is an event
    if (Looper.thisState() != tState::Event)
        return;
    
    int* value = (int*)Looper.eventData();
    if (value) {
        Serial.printf("[Action] Event received! Value: %d\n", *value);
        
        LP_DELAY(500);
        Serial.println("[Action] Processing...");
        
        LP_DELAY(500);
        Serial.println("[Action] Done!");
    }
});

// Timer that sends events to the action thread
LP_TIMER(2000, []() {
    static int counter = 0;
    counter++;
    
    Serial.printf("\n[Timer] Sending event to 'action': %d\n", counter);
    LP_SEND_EVENT("action", &counter);
});

// Another thread with state management
LP_THREAD_("worker", {
    switch (Looper.thisState()) {
        case tState::Setup:
            Serial.println("[Worker] Setup - initializing");
            break;
            
        case tState::Loop:
            // Normal work
            static uint32_t loopCount = 0;
            if (++loopCount % 1000000 == 0) {
                Serial.printf("[Worker] Loop iteration: %lu\n", loopCount);
            }
            break;
            
        case tState::Event:
            Serial.println("[Worker] Event received!");
            int* data = (int*)Looper.eventData();
            if (data) {
                Serial.printf("[Worker] Event data: %d\n", *data);
            }
            break;
            
        case tState::Exit:
            Serial.println("[Worker] Exiting");
            break;
    }
    
    LP_DELAY(10);
});

// Timer to send events to worker
LP_TIMER(3000, []() {
    static int value = 100;
    value += 10;
    
    Serial.printf("\n[Timer] Sending event to 'worker': %d\n", value);
    LP_SEND_EVENT("worker", &value);
});

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP-Looper: Event-Driven Thread Demo ===");
    Serial.println("This demo shows threads receiving events and state management\n");
    
    ESP_LOOPER.begin();
    
    Serial.println("All tasks started!");
    Serial.println("Watch events being sent to threads\n");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
