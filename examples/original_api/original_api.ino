#include <ESPLooper.h>

// ===== TICKER - runs continuously =====
LP_TICKER([]() {
    static uint32_t counter = 0;
    if (++counter % 100000 == 0) {
        Serial.printf("[Ticker] Counter: %lu\n", counter);
    }
});

// ===== TIMER - periodic execution =====
LP_TIMER_("timer_1sec", 1000, []() {
    Serial.println("[Timer] 1 second passed");
    int tick = 1;
    LP_SEND_EVENT("tick", &tick);
});

// ===== THREAD - async operations with Duff's Device =====
LP_THREAD({
    Serial.println("[Thread] Starting blink");
    pinMode(LED_BUILTIN, OUTPUT);
    
    while (true) {
        digitalWrite(LED_BUILTIN, HIGH);
        LP_DELAY(500);
        
        digitalWrite(LED_BUILTIN, LOW);
        LP_DELAY(500);
    }
});

// ===== THREAD with conditions =====
LP_THREAD_("conditional", {
    Serial.println("[Conditional Thread] Waiting for Serial data...");
    
    LP_WAIT(Serial.available() > 0);
    
    Serial.println("[Conditional Thread] Data received!");
    while (Serial.available()) {
        Serial.write(Serial.read());
    }
    Serial.println();
    
    LP_DELAY(1000);
    LP_RESTART();  // Start over
});

// ===== THREAD with counter =====
LP_THREAD({
    static int i;
    
    for (i = 0; i < 10; i++) {
        Serial.printf("[Counter Thread] i = %d\n", i);
        LP_DELAY(300);
    }
    
    Serial.println("[Counter Thread] Finished, restarting...");
    LP_DELAY(2000);
    LP_RESTART();
});

// ===== EVENT LISTENER =====
LP_LISTENER_("tick", [](const ESPLooper::Event& evt) {
    Serial.println("[Listener] Timer event received!");
});

// ===== SEMAPHORE EXAMPLE =====
LP_SEM dataSem = xSemaphoreCreateBinary();
int sharedData = 0;

LP_THREAD_("producer", {
    while (true) {
        LP_DELAY(800);
        sharedData = random(0, 100);
        Serial.printf("[Producer] Generated: %d\n", sharedData);
        LP_SEM_SIGNAL(dataSem);
    }
});

LP_THREAD_("consumer", {
    while (true) {
        LP_SEM_WAIT(dataSem);
        Serial.printf("[Consumer] Consumed: %d\n", sharedData);
    }
});

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP-Looper: Original API Demo ===");
    Serial.println("All original Looper macros working with FreeRTOS!\n");
    
    // Initialize semaphore
    xSemaphoreGive(dataSem);
    
    ESP_LOOPER.begin();
    
    Serial.println("All tasks started!");
    Serial.println("Type something in Serial Monitor...\n");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
