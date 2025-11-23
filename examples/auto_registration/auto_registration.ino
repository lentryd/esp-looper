#include <ESPLooper.h>

// Global auto-registered timer - no need to call in setup()!
// Runs every 1 second on core 0
LP_TIMER(1000, []() {
    static int counter = 0;
    Serial.printf("[Core %d] Timer tick #%d\n", xPortGetCoreID(), counter++);
    
    int data = random(0, 100);
    LP_SEND_EVENT(EVENT_ID("sensor"), &data, sizeof(data));
}, true, 0); // autoStart=true, coreId=0

// Named timer for better identification
LP_TIMER_NAMED(fastTimer, 250, []() {
    Serial.printf("[Core %d] Fast timer!\n", xPortGetCoreID());
}, true, 1); // Run on core 1

// Global auto-registered listener
LP_LISTENER(EVENT_ID("sensor"), [](const ESPLooper::Event& evt) {
    int value = *(int*)evt.data;
    Serial.printf("[Core %d] Listener received: %d\n", xPortGetCoreID(), value);
    
    // Send another event
    if (value > 50) {
        LP_SEND_EVENT(EVENT_ID("high_value"), &value, sizeof(value));
    }
}, 1); // Run on core 1

// Another listener for high values
LP_LISTENER_NAMED(alertListener, EVENT_ID("high_value"), 
    [](const ESPLooper::Event& evt) {
        int value = *(int*)evt.data;
        Serial.printf("⚠️  HIGH VALUE ALERT: %d\n", value);
    });

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== ESP-Looper Auto-Registration Example ===");
    Serial.println("All tasks are defined globally and auto-registered!\n");
    
    // Just call begin() - all global tasks are automatically initialized
    ESP_LOOPER.begin();
    
    // You can still add tasks dynamically if needed
    ESP_TIMER("dynamic", 2000, []() {
        Serial.println("[Dynamic] This task was added in setup()");
    });
    
    Serial.println("ESP-Looper started with auto-registered tasks!");
    Serial.println("Watch the magic happen...\n");
}

void loop() {
    // Not used with ESP-Looper
    vTaskDelay(portMAX_DELAY);
}
