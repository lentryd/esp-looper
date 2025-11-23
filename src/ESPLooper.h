#pragma once

// ESP-Looper: Multi-threaded Event-Driven Framework for ESP32
// Version: 1.0.0
// Author: lentryd
// License: MIT

#include "Event.h"
#include "Task.h"
#include "Looper.h"

// Usage example:
// 
// #include <ESPLooper.h>
//
// void setup() {
//     ESP_LOOPER.begin();
//     
//     ESP_TIMER("sensor", 1000, []() {
//         int data = analogRead(34);
//         ESP_SEND_EVENT(EVENT_ID("data"), &data, sizeof(data));
//     });
//     
//     ESP_LISTENER("display", EVENT_ID("data"), [](const ESPLooper::Event& evt) {
//         Serial.printf("Data: %d\n", *(int*)evt.data);
//     });
// }