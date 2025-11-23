#include <ESPLooper.h>

struct SensorData {
    float temperature;
    float humidity;
    uint32_t timestamp;
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== ESP-Looper Multi-Core Example ===\n");
    
    ESP_LOOPER.begin();
    
    // Sensor task on core 0 - reads sensors
    auto sensorTask = ESP_TIMER("sensor", []() {
        static SensorData data;
        data.temperature = random(200, 300) / 10.0;
        data.humidity = random(400, 600) / 10.0;
        data.timestamp = millis();
        
        Serial.printf("[Core %d] Sensor: T=%.1f°C H=%.1f%%\n",
                     xPortGetCoreID(), data.temperature, data.humidity);
        
        ESP_SEND_EVENT(EVENT_ID("sensor_data"), &data, sizeof(SensorData));
    }, 500, true, 0);
    
    // Processing task on core 1 - processes data
    ESP_LISTENER("processor", EVENT_ID("sensor_data"), 
        [](const ESPLooper::Event& evt) {
            SensorData* data = (SensorData*)evt.data;
            
            Serial.printf("[Core %d] Processing: T=%.1f°C H=%.1f%%\n",
                         xPortGetCoreID(), data->temperature, data->humidity);
            
            // Alert if temperature too high
            if (data->temperature > 25.0) {
                ESP_SEND_EVENT(EVENT_ID("high_temp_alert"), data, sizeof(SensorData));
            }
        }, 1);
    
    // Alert handler - can run on any core
    ESP_LISTENER("alert", EVENT_ID("high_temp_alert"), 
        [](const ESPLooper::Event& evt) {
            SensorData* data = (SensorData*)evt.data;
            Serial.printf("⚠️  [Core %d] HIGH TEMPERATURE ALERT: %.1f°C\n",
                         xPortGetCoreID(), data->temperature);
        });
    
    // Stats printer every 5 seconds
    ESP_TIMER("stats", []() {
        Serial.println("\n--- Statistics ---");
        ESP_LOOPER.printStats();
        Serial.println("------------------\n");
    }, 5000, true, tskNO_AFFINITY);
    
    Serial.println("Multi-core tasks started!\n");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}