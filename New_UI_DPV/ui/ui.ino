#include <Arduino.h>
#include <ESP_Panel_Library.h>

#include <lvgl.h>
#include "lvgl_v8_port.h"
// #include <demos/lv_demos.h>

#include <ui.h>
using namespace esp_panel::drivers;
using namespace esp_panel::board;

/**
 * To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 */
 // #include <demos/lv_demos.h>
 // #include <examples/lv_examples.h>

void setup()
{
    String title = "LVGL porting example";

    Serial.begin(115200);

    Serial.println("Initializing board");
    // หมายเหตุ: การเรียก board->init() จะสำเร็จ เพราะเราได้ตั้งค่า ESP_OPEN_TOUCH เป็น 0 แล้ว
    Board *board = new Board();
    board->init();

    #if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    // คำสั่ง assert นี้ควรจะสำเร็จแล้ว เนื่องจาก Touch ถูกปิดการใช้งาน
    assert(board->begin());

    Serial.println("Initializing LVGL");
    // board->getTouch() จะคืนค่าเป็น nullptr เมื่อ ESP_OPEN_TOUCH เป็น 0
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);

    // ... (ส่วน Demo ที่ Commented Out) ...

    /* ต้องเรียกใช้ ui_init() ในขณะที่ Mutex ถูกล็อก */
    ui_init();

    /* Release the mutex */
    lvgl_port_unlock();
}

void loop()
{
    // **สำคัญที่สุด:** ต้องเรียกใช้ lv_timer_handler() เพื่อให้ LVGL วาดหน้าจอและอัปเดต
    // ล็อก Mutex ก่อนเรียกใช้ LVGL API
    lvgl_port_lock(0); 
    lv_timer_handler();
    lvgl_port_unlock();

    // ใช้ delay สั้นๆ เพื่อให้ CPU มีเวลาทำงานอื่น ๆ
    delay(5); 
}