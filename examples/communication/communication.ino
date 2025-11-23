#include <ESPLooper.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== ESP-Looper Communication Example ===\n");
    
    ESP_LOOPER.begin();
    
    // Create multiple producer tasks on core 0
    for (int i = 0; i < 3; i++) {
        char name[16];
        snprintf(name, sizeof(name), "producer_%d", i);
        
        ESP_TIMER(name, [i]() {
            static int counter = 0;
            char msg[64];
            snprintf(msg, sizeof(msg), "Message #%d from producer %d", counter++, i);
            
            Serial.printf("[Core %d] %s sending: %s\n", 
                         xPortGetCoreID(), name, msg);
            
            ESP_LOOPER.sendEvent("message", msg, strlen(msg) + 1, true);
        }, 1000 + i * 500, true, 0);
    }
    
    // Consumer task on core 1
    ESP_LISTENER("consumer", EVENT_ID("message"), 
        [](const ESPLooper::Event& evt) {
            Serial.printf("[Core %d] Consumer received: %s\n",
                         xPortGetCoreID(), (char*)evt.data);
        }, 1);
    
    // Global event monitor (receives ALL events)
    ESP_LOOPER.events().onAny([](const ESPLooper::Event& evt) {
        Serial.printf("[Monitor Core %d] Event ID: %u, Size: %d bytes\n",
                     xPortGetCoreID(), evt.id, evt.dataSize);
    });
    
    Serial.println("Communication tasks started!\n");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}