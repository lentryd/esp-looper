#include <ESPLooper.h>

// LED timer with state management
LP_TIMER_("led_timer", 500, []() {
    static bool state = false;
    
    switch (Looper.thisState()) {
        case tState::Setup:
            pinMode(LED_BUILTIN, OUTPUT);
            Serial.println("[LED] Setup");
            break;
            
        case tState::Loop:
            state = !state;
            digitalWrite(LED_BUILTIN, state);
            Serial.println(state ? "ON" : "OFF");
            break;
            
        case tState::Event:
            Serial.println("[LED] Event received!");
            break;
            
        case tState::Exit:
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("[LED] Exit");
            break;
    }
});

// Control timer - toggles LED timer every second
LP_TIMER(1000, []() {
    // Toggle LED timer every second
    auto ledTimer = Looper["led_timer"];
    if (ledTimer) {
        ledTimer->toggle();
        Serial.print("LED Timer: ");
        Serial.println(ledTimer->isEnabled() ? "Enabled" : "Disabled");
    }
});

// Status ticker - shows task info
LP_TICKER_("status", []() {
    static uint32_t counter = 0;
    if (++counter % 500000 == 0) {
        auto ledTimer = Looper["led_timer"];
        if (ledTimer) {
            Serial.printf("[Status] LED timer state: %s\n", 
                         ledTimer->isEnabled() ? "Enabled" : "Disabled");
        }
    }
});

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP-Looper: Task Control Demo ===");
    Serial.println("This demo shows task enable/disable/toggle control\n");
    
    ESP_LOOPER.begin();
    
    Serial.println("All tasks started!");
    Serial.println("Watch the LED timer being toggled every second\n");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
