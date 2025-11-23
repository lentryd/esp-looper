#pragma once

// ESP-Looper: Multi-threaded Event-Driven Framework for ESP32
// Version: 1.1.0
// Author: lentryd
// License: MIT

#include "Event.h"
#include "Task.h"
#include "Looper.h"
#include "AutoTask.h"

// Usage example with auto-registration:
// 
// #include <ESPLooper.h>
//
// // Global auto-registered timer (no need to call in setup!)
// LP_TIMER(1000, []() {
//     int data = analogRead(34);
//     LP_SEND_EVENT(EVENT_ID("data"), &data, sizeof(data));
// });
//
// // Global auto-registered listener
// LP_LISTENER(EVENT_ID("data"), [](const ESPLooper::Event& evt) {
//     Serial.printf("Data: %d\n", *(int*)evt.data);
// });
//
// void setup() {
//     Serial.begin(115200);
//     ESP_LOOPER.begin(); // Automatically initializes all tasks
// }
//
// void loop() {
//     vTaskDelay(portMAX_DELAY);
// }