#include "sensors.h"
#include <DHT.h>
#include <ui.h>       // To access UI widgets like ui_Temp_Label
#include <lvgl.h>     // For LVGL locking

// --- Configuration ---
#define DHTTYPE    DHT22   // Change to DHT11 if needed
#define UPDATE_INTERVAL_MS 2000 // Read every 2 seconds

// --- Global Objects ---
DHT dht(PIN_DHT_SENSOR, DHTTYPE);

// --- Variables ---
static float current_temp = 0.0;
static float current_humid = 0.0;
static unsigned long last_update = 0;

void sensors_init() {
    Serial.println("Initializing Sensors...");
    dht.begin();
    pinMode(PIN_DHT_SENSOR, INPUT_PULLUP); // Safety for standalone sensors
}

void sensors_update() {
    // Non-blocking delay
    if (millis() - last_update > UPDATE_INTERVAL_MS) {
        last_update = millis();

        // 1. Read Data
        float t = dht.readTemperature();
        float h = dht.readHumidity();

        // 2. Validate
        if (isnan(t) || isnan(h)) {
            Serial.println("Failed to read from DHT sensor!");
            return; // Keep old values
        }

        current_temp = t;
        current_humid = h;
        
        Serial.printf("Temp: %.1f C, Humid: %.1f %%\n", t, h);

        // 3. Update UI (Thread Safe)
        // Note: You must ensure ui_init() was called before this
        lvgl_port_lock(0);
        
        // Example: Update Label if it exists
        // if (ui_Temp_Label != NULL) {
        //    lv_label_set_text_fmt(ui_Temp_Label, "%.1f C", current_temp);
        // }
        
        // Temporary: Show on ui_time if ui_Temp_Label doesn't exist
        if (ui_time != NULL) {
             lv_label_set_text_fmt(ui_time, "%.1f C", current_temp);
        }

        lvgl_port_unlock();
    }
}

float get_temperature() {
    return current_temp;
}

float get_humidity() {
    return current_humid;
}
