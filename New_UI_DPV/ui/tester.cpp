#include "tester.h"
#include "sensors.h"
#include <Arduino.h>
#include <ui.h>
#include <lvgl.h>
#include "lvgl_v8_port.h"

// Simulated data state
static float depth = 12.5;
static float speed = 3.2;
static int gear = 1;
static int battery = 85;
static float temperature = 26.5;
static float uv_index = 0.5;
static uint32_t last_update_time = 0;

void tester_init() {
    randomSeed(analogRead(0)); 
    Serial.println("UI Random Tester Initialized.");
}

void tester_update() {
    // Update every 1 second
    if (millis() - last_update_time >= 1000) {
        last_update_time = millis();

        // 1. Generate random changes
        depth += (random(-5, 6) / 10.0);
        if (depth < 0) depth = 0;
        
        speed += (random(-2, 3) / 10.0);
        if (speed < 0) speed = 0;
        
        battery -= random(0, 2);
        if (battery < 0) battery = 100;

        temperature += (random(-1, 2) / 10.0);
        uv_index += (random(-2, 3) / 10.0);
        if (uv_index < 0) uv_index = 0;
        
        gear = (int)random(1, 5);

        Serial.printf("[Tester] D:%.1fm, S:%.1fkn, G:%d, B:%d%%, T:%.1fC, UVI:%.1f\n", 
                      depth, speed, gear, battery, temperature, uv_index);

        // 2. Update UI (Thread Safe)
        if (lvgl_port_lock(0)) {
            if (ui_DEPTH != NULL) {
                lv_label_set_text_fmt(ui_DEPTH, "%.1f", depth);
            }
            if (ui_SPEED != NULL) {
                lv_label_set_text_fmt(ui_SPEED, "%.1f", speed);
            }
            if (ui_GEAR != NULL) {
                lv_label_set_text_fmt(ui_GEAR, "%d", gear);
            }
            if (ui_BATTERY != NULL) {
                lv_label_set_text_fmt(ui_BATTERY, "%d%%", battery);
            }
            if (ui_TEMP != NULL) {
                lv_label_set_text_fmt(ui_TEMP, "%.1f", temperature);
            }
            // Note: If you have a UI label for UV Index, add it here.
            // Example:
            // if (ui_uv_label != NULL) {
            //    lv_label_set_text_fmt(ui_uv_label, "%.1f", uv_index);
            // }
            lvgl_port_unlock();
        }
    }
}
