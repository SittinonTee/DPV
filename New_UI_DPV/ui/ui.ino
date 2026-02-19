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
    delay(1000); // เพิ่มเวลาให้ Serial Monitor เชื่อมต่อทัน
    
    printf("\n\n--- [DPV] DUAL-BOARD MODE: DISPLAY MASTER ---\n");
    
    // พิมพ์ค่า MAC ADDRESS วนซ้ำ 5 รอบเพื่อความชัวร์
    WiFi.mode(WIFI_STA);

    String mac = WiFi.macAddress();
    Serial.print(">>> DEVICE MAC ADDRESS: ");
    Serial.println(mac);
    printf(">>> DEVICE MAC ADDRESS: %s\n", mac.c_str());
    
    // for(int i=0; i<5; i++) {
    //     String mac = WiFi.macAddress();
    //     Serial.print(">>> DEVICE MAC ADDRESS: ");
    //     Serial.println(mac);
    //     printf(">>> DEVICE MAC ADDRESS: %s\n", mac.c_str());
    //     delay(500);
    // }
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
        if (ui_DIRECTION) {
            lv_label_set_text(ui_DIRECTION, "READY");
            printf("DEBUG: ตัวแปร ui_DIRECTION พร้อมใช้งาน\n");
        } else {
            printf("DEBUG: เกิดข้อผิดพลาด! ui_DIRECTION เป็นค่าว่าง\n");
        }
        lvgl_port_unlock();
    }

    printf("--- ตั้งค่าระบบเสร็จสิ้น - เริ่มต้นการทำงาน (Entering Loop) ---\n");
}

// ฟังก์ชันสำหรับอัปเดตข้อมูลเซ็นเซอร์ลงบนหน้าจอ LCD
void update_ui_labels() {
    static bool was_connected = true;
    bool connected = sensors_is_connected();

    // เพิ่มประสิทธิภาพ: อัปเดตเฉพาะเมื่อมีข้อมูลใหม่ หรือสถานะการเชื่อมต่อเปลี่ยน
    if (!sensors_new_data_available() && connected == was_connected) {
        return; 
    }
    was_connected = connected;

    // 1. ดึงข้อมูลล่าสุด
    SensorData data = sensors_get_data();
    
    // 2. ล็อค LVGL Mutex
    if (lvgl_port_lock(50)) {
        
        // --- [color status] ---
        lv_color_t color_normal = lv_color_hex(0xFFFFFF); // สีขาวปกติ (ปรับตามดีไซน์เดิม)
        lv_color_t color_green = lv_color_hex(0x32FF32); // เปลี่ยนเป็นสีเขียวตามคำขอ
        lv_color_t color_error  = lv_color_hex(0xFF0000); // สีแดงแจ้งเตือน

        // [Direction/เข็มทิศ]
        if (ui_DIRECTION) {
            bool is_error = (!connected || !data.bmp_online);
            lv_obj_set_style_text_color(ui_DIRECTION, is_error ? color_error : color_green, 0);
            
            if (is_error) {
                lv_label_set_text(ui_DIRECTION, "E");
            } else {
                float d = data.direction;
                String val;
                if (abs(fmod(d, 45.0)) < 0.1 || abs(fmod(d, 45.0) - 45.0) < 0.1) {
                    const char* cardinal[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
                    int index = (int)((d + 22.5) / 45) % 8;
                    val = cardinal[index];
                } else {
                    val = String(d, 1) + "°";
                }
                lv_label_set_text(ui_DIRECTION, val.c_str());
            }
        }
        

        // [Temperature/อุณหภูมิ]
        if (ui_TEMP) {
            bool is_error = (!connected || !data.bmp_online);
            lv_obj_set_style_text_color(ui_TEMP, is_error ? color_error : color_green, 0);
            if (is_error) lv_label_set_text(ui_TEMP, "E");
            else lv_label_set_text(ui_TEMP, String(data.temperature, 1).c_str());
        }

        // [Depth/ความลึก]
        if (ui_DEPTH) {
            bool is_error = (!connected);
            lv_obj_set_style_text_color(ui_DEPTH, is_error ? color_error : lv_color_hex(0xFFFFFF), 0);
            
            if (is_error) {
                lv_label_set_text(ui_DEPTH, "E");
            } else {
                lv_label_set_text(ui_DEPTH, String(data.depth, 1).c_str());
            }
            
            // แจ้งเตือนความลึกเกิน 40 เมตร
            if (ui_ICONDEPTH) {
                if (!connected) {
                    lv_obj_set_style_img_recolor(ui_ICONDEPTH, color_error, 0);
                    lv_obj_set_style_img_recolor_opa(ui_ICONDEPTH, 255, 0);
                } else if (data.direction >= 40.0) {
                    lv_obj_set_style_img_recolor(ui_ICONDEPTH, color_error, 0);
                    lv_obj_set_style_img_recolor_opa(ui_ICONDEPTH, 255, 0);
                } else {
                    lv_obj_set_style_img_recolor_opa(ui_ICONDEPTH, 0, 0);
                }
            }
        }

        // [Speed/ความเร็ว]
        if (ui_SPEED) {
            lv_obj_set_style_text_color(ui_SPEED, !connected ? color_error : lv_color_hex(0xB8B8B8), 0);
            if (!connected) lv_label_set_text(ui_SPEED, "E");
            else lv_label_set_text(ui_SPEED, String(data.speed).c_str());
        }

        // [Battery]
        if (ui_BATTERY) {
            lv_obj_set_style_text_color(ui_BATTERY, !connected ? color_error : lv_color_hex(0x808080), 0);
            if (!connected) lv_label_set_text(ui_BATTERY, "E");
            else {
                String val = String(data.battery) + "%";
                lv_label_set_text(ui_BATTERY, val.c_str());
            }
        }

        // [Time] (เวลาไม่ต้องขึ้น E เพราะนับจากบอร์ดตัวเอง แต่เปลี่ยนเป็นสีแดงได้ถ้าสัญญาณหลุด)
        if (ui_TIME) {
            lv_obj_set_style_text_color(ui_TIME, !connected ? color_error : color_green, 0);
            unsigned long total_minutes = millis() / 60000;
            String val = String(total_minutes) + " min";
            lv_label_set_text(ui_TIME, val.c_str());
        }
        
        // 3. ปลดล็อค LVGL Mutex
        lvgl_port_unlock();
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
        bool connected = sensors_is_connected();
        
        if (connected) {
            printf("[ONLINE] D:%.1f, T:%.1f, P:%.1f | ID:%u | BNO:%s BMP:%s\n", 
                   data.direction, data.temperature, data.pressure, data.packet_id,
                   data.bno_online ? "OK" : "ERR", data.bmp_online ? "OK" : "ERR");
        } else {
            printf("[OFFLINE] กำลังรอสัญญาณจากบอร์ดเซ็นเซอร์... (Signal Lost)\n");
        }
        last_id = millis();
    }

    delay(20); 
}
