#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include <WiFi.h>
#include "lvgl_v8_port.h"
#include "sensors.h"
#include "esp_log.h"
#include <ui.h>

using namespace esp_panel::drivers;
using namespace esp_panel::board;

void setup()
{
    Serial.begin(115200);
    delay(5000); // เพิ่มเวลาให้ Serial Monitor เชื่อมต่อทัน
    
    printf("\n\n--- [DPV] DUAL-BOARD MODE: DISPLAY MASTER ---\n");
    
    // พิมพ์ค่า MAC ADDRESS วนซ้ำ 5 รอบเพื่อความชัวร์
    WiFi.mode(WIFI_STA);
    for(int i=0; i<5; i++) {
        String mac = WiFi.macAddress();
        Serial.print(">>> DEVICE MAC ADDRESS: ");
        Serial.println(mac);
        printf(">>> DEVICE MAC ADDRESS: %s\n", mac.c_str());
        delay(500);
    }
    Serial.println("----------------------------------------------");

    // 1. Initialize display board hardware
    Board *board = new Board();
    board->init();
    if (!board->begin()) {
        printf("!!! CRITICAL: Board begin FAILED !!!\n");
        while(1) delay(100);
    }
    printf("Board hardware begin SUCCESS\n");

    // 2. Initialize ESP-NOW Receiver
    printf("Initializing ESP-NOW...\n");
    sensors_init();

    // 3. Initialize Graphics Layer (LVGL Port)
    // 3. เริ่มต้นการทำงานของ Graphics Layer (LVGL Port)
    printf("Initializing LVGL Port...\n");
    lvgl_port_init(board->getLCD(), NULL); 
    printf("LVGL Port init finished in main thread\n");

    vTaskDelay(pdMS_TO_TICKS(500)); 

    // 4. สร้างหน้าจอ UI จาก SquareLine Studio
    printf("MAIN: กำลังสร้างหน้าจอ UI (ui_init)...\n");
    lvgl_port_lock(-1);
    ui_init(); 
    lvgl_port_unlock();
    printf("MAIN: สร้างหน้าจอ UI สำเร็จ\n");
    
    // ขั้นตอนทดสอบการทำงานของหน้าจอ: ลองเขียนค่า READY หนึ่งครั้ง
    if (lvgl_port_lock(100)) {
        if (ui_direction) {
            lv_label_set_text(ui_direction, "READY");
            printf("DEBUG: ตัวแปร ui_direction พร้อมใช้งาน\n");
        } else {
            printf("DEBUG: เกิดข้อผิดพลาด! ui_direction เป็นค่าว่าง\n");
        }
        lvgl_port_unlock();
    }

    printf("--- ตั้งค่าระบบเสร็จสิ้น - เริ่มต้นการทำงาน (Entering Loop) ---\n");
}

// ฟังก์ชันสำหรับอัปเดตข้อมูลเซ็นเซอร์ลงบนหน้าจอ LCD
void update_ui_labels() {
    // 1. ดึงข้อมูลล่าสุดจากตัวรับสัญญาณไร้สาย Sensors Driver
    SensorData data = sensors_get_data();
    
    // 2. ล็อค LVGL Mutex ก่อนแก้ไขหน้าจอ (สำคัญมากสำหรับการทำงานแบบ Multi-thread)
    if (lvgl_port_lock(50)) {
        
        // [Direction/เข็มทิศ] ใช้ค่า Heading (0-360 องศา)
        // ตั้งเป็นสีแดง (Red) เพื่อทดสอบการอัปเดตหน้าจอ
        if (ui_direction) {
            lv_obj_set_style_text_color(ui_direction, lv_color_hex(0xFF0000), 0);
            lv_label_set_text(ui_direction, String(data.heading, 0).c_str());
            lv_obj_invalidate(ui_direction); // บังคับให้จอวาดใหม่เฉพาะจุดนี้
        }
        
        // [Temperature/อุณหภูมิ] นำค่ามาจากบอร์ดเซ็นเซอร์
        if (ui_temp) {
            lv_obj_set_style_text_color(ui_temp, lv_color_hex(0xFF0000), 0);
            lv_label_set_text(ui_temp, String(data.temperature, 1).c_str());
            lv_obj_invalidate(ui_temp);
        }

        // [Depth/ความลึก] แสดงผลในหน่วยเมตร
        if (ui_deep) {
            lv_label_set_text(ui_deep, String(data.depth, 1).c_str());
            lv_obj_invalidate(ui_deep);
        }

        // [Speed/ความเร็ว] แสดงค่าความเร็วที่ส่งมาจากบอร์ดเซ็นเซอร์
        if (ui_speed) {
            lv_label_set_text(ui_speed, String(data.speed, 1).c_str());
            lv_obj_invalidate(ui_speed);
        }

        // [Pressure/ความดันอากาศ] ใช้ช่องแสดงผล Percentage ในการโชว์ค่า hPa
        if (ui_percentage) {
            lv_label_set_text(ui_percentage, String(data.pressure, 0).c_str());
            lv_obj_invalidate(ui_percentage);
        }

        // [Pitch & Roll / องศาการเอียง] รวมค่า Pitch และ Roll มาแสดงในช่อง Time
        if (ui_time) {
            String pr = "P:" + String(data.pitch, 0) + " R:" + String(data.roll, 0);
            lv_label_set_text(ui_time, pr.c_str());
            lv_obj_invalidate(ui_time);
        }
        
        // 3. ปลดล็อค LVGL Mutex
        lvgl_port_unlock();
    } else {
        // หากระบบวาดภาพประมวลผลไม่ทันและถือล็อคไว้นานเกินไป จะแจ้งเตือนที่นี่
        static unsigned long last_warn = 0;
        if(millis() - last_warn > 5000) {
            printf("คำเตือน: LVGL Lock Timeout! ระบบวาดภาพอาจจะค้าง\n");
            last_warn = millis();
        }
    }
}

void loop()
{
    // อัปเดตข้อมูลเซ็นเซอร์ที่ได้รับแบบไร้สายลงบนหน้าจอ LCD
    update_ui_labels();

    // ระบบ Heartbeat: รายงานสถานะและค่าปัจจุบันออกทาง Serial ทุก 2 วินาที
    static unsigned long last_id = 0;
    if (millis() - last_id > 2000) {
        SensorData data = sensors_get_data();
        if (data.bno_online || data.bmp_online) {
            printf("[ข้อมูล] ทิศทาง:%.1f, อุณหภูมิ:%.1f, ความดัน:%.1f | P:%.0f R:%.0f | ID:%u\n", 
                   data.heading, data.temperature, data.pressure, 
                   data.pitch, data.roll, data.packet_id);
        } else {
            printf("[รอข้อมล] กำลังรอสัญญาณจากบอร์ดเซ็นเซอร์...\n");
        }
        last_id = millis();
    }

    delay(20); 
}
