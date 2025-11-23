#include <ESPLooper.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== ESP-Looper Basic Example ===");
    
    // Initialize framework
    ESP_LOOPER.begin();
    
    // Create timer task that runs every second on core 0
    ESP_TIMER("sensor", 1000, []() {
        int temp = random(20, 30);
        Serial.printf("[Core %d] Sensor reading: %d°C\n", 
                     xPortGetCoreID(), temp);
        
        // Send event with data
        ESP_SEND_EVENT(EVENT_ID("temperature"), &temp, sizeof(temp));
    }, true, 0); // autoStart=true, coreId=0
    
    // Create listener on core 1 that receives events
    ESP_LISTENER("display", EVENT_ID("temperature"), [](const ESPLooper::Event& evt) {
        int temp = *(int*)evt.data;
        Serial.printf("[Core %d] Display update: %d°C\n", 
                     xPortGetCoreID(), temp);
    }, 1); // coreId=1
    
    Serial.println("ESP-Looper started!");
    Serial.println("Watch events flowing between cores...\n");
}

void loop() {
    // Arduino loop is not used with ESP-Looper
    vTaskDelay(portMAX_DELAY);
}