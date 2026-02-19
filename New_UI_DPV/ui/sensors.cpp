#include "sensors.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

static SensorData received_data = {0};
static SemaphoreHandle_t data_mux = NULL;
static bool has_new_data = false;
static uint32_t last_received_time = 0; // เก็บเวลาที่ได้รับข้อมูลล่าสุด

// ฟังก์ชันจัดการเมื่อได้รับข้อมูลไร้สาย (ESP-NOW Callback)
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    if (len == sizeof(SensorData)) {
        // ใช้ Semaphore แบบไม่รอ (Timeout = 0) เพื่อไม่ให้หน่วง WiFi Task
        if (data_mux != NULL && xSemaphoreTake(data_mux, 0)) {
            memcpy(&received_data, incomingData, sizeof(SensorData));
            has_new_data = true;
            last_received_time = millis(); // อัปเดตเวลาล่าสุดที่ได้รับข้อมูล
            xSemaphoreGive(data_mux);
        }
    } else {
        // แจ้งเตือนด้วย ESP_LOGW แทน printf เพื่อความปลอดภัยใน callback
        static unsigned long last_warn = 0;
        if(millis() - last_warn > 5000) {
            ESP_LOGW("SENSORS", "ขนาดข้อมูล ESP-NOW ไม่ตรงกัน! ได้รับ: %d, คาดหวัง: %d", len, (int)sizeof(SensorData));
            last_warn = millis();
        }
    }
}

// ฟังก์ชันเริ่มต้นระบบ WiFi และตัวรับ ESP-NOW บน Channel 1
void sensors_init() {
    // สร้าง Mutex เพื่อปกป้องตัวแปร 'received_data' จากการเข้าถึงพร้อมกัน
    data_mux = xSemaphoreCreateMutex();
    
    // ตั้งค่า WiFi เป็นโหมด Station. สำคัญ: ต้องล็อคความถี่ไว้ที่ Channel 1
    // เพราะบอร์ดทั้งสองต้องอยู่บนความถี่วิทยุเดียวกันจึงจะคุยกันได้
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // เริ่มต้นโปรโตคอล ESP-NOW
    if (esp_now_init() != ESP_OK) {
        ESP_LOGE("SENSORS", "เกิดข้อผิดพลาดในการเริ่มต้น ESP-NOW");
        return;
    }

    // ลงทะเบียนฟังก์ชัน 'OnDataRecv' ให้ทำงานทุกครั้งที่มีข้อมูลส่งมาจากบอร์ดเซ็นเซอร์
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    
    ESP_LOGI("SENSORS", "เริ่มต้นตัวรับ ESP-NOW สำเร็จบน Channel 1");
}

void sensors_update() {
    // ฟังก์ชันนี้ปัจจุบันไม่ได้ใช้งานเนื่องจากใช้ระบบ Callback
}

// ฟังก์ชันสำหรับดึงข้อมูลเซ็นเซอร์ล่าสุดออกมาใช้งาน (ปลอดภัยต่อการทำงานแบบ Multi-thread)
SensorData sensors_get_data() {
    SensorData current_data = {0};
    if (data_mux != NULL && xSemaphoreTake(data_mux, pdMS_TO_TICKS(50))) {
        current_data = received_data;
        // การดึงข้อมูลไปใช้จะเคลียร์ flag ว่าข้อมูลใหม่ถูกอ่านแล้ว
        has_new_data = false; 
        xSemaphoreGive(data_mux);
    }
    return current_data;
}

// ฟังก์ชันเช็คว่ามีข้อมูลใหม่ที่ยังไม่ได้อ่านหรือไม่
bool sensors_new_data_available() {
    return has_new_data;
}

// ฟังก์ชันเช็คว่าบอร์ดส่งยังเชื่อมต่ออยู่หรือไม่ (Timeout 3 วินาที)
bool sensors_is_connected() {
    if (last_received_time == 0) return false;
    return (millis() - last_received_time < 3000);
}
