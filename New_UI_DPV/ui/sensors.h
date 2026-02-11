#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// Sensor Pin Definitions
#define PIN_DHT_SENSOR  6   // GPIO 6 (SENSOR Port)
#define PIN_I2C_SDA     8
#define PIN_I2C_SCL     9

// Initializer
void sensors_init();

// Update function (Call this in loop)
void sensors_update();

// Getters (For UI to read)
float get_temperature();
float get_humidity();

#endif
