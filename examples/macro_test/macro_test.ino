#include <ESPLooper.h>

// Test all macro variations from the problem statement

// ===== Test unnamed versions =====
LP_TICKER([]() {
    // Unnamed ticker - should use __LINE__ for unique naming
});

LP_TIMER(1000, []() {
    // Unnamed timer - should use __LINE__ for unique naming
});

LP_THREAD({
    // Unnamed thread - should use __LINE__ for unique naming
    LP_DELAY(100);
});

// ===== Test named versions (with string literals) =====
LP_TICKER_("my_ticker", []() {
    // Named ticker with string ID - should not try to concatenate with ##
});

LP_TIMER_("my_timer", 1000, []() {
    // Named timer with string ID - should not try to concatenate with ##
});

LP_THREAD_("my_thread", {
    // Named thread with string ID - should not try to concatenate with ##
    LP_DELAY(100);
});

LP_LISTENER_("event", [](const ESPLooper::Event& evt) {
    // Named listener with string ID - should not try to concatenate with ##
});

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP-Looper: Macro Test ===");
    Serial.println("Testing all macro variations...\n");
    
    // Test event sending with data
    int value = 42;
    LP_SEND_EVENT("test", &value);
    
    // Test event sending with null (should handle null properly)
    LP_SEND_EVENT("test", nullptr);
    
    // Test LP_PUSH_EVENT with data
    LP_PUSH_EVENT("test2", &value);
    
    // Test LP_PUSH_EVENT with null
    LP_PUSH_EVENT("test2", nullptr);
    
    ESP_LOOPER.begin();
    
    Serial.println("All macros compiled successfully!");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
