#include "sensors.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

static SensorData received_data = {0};
static SemaphoreHandle_t data_mux = NULL;

// ฟังก์ชันจัดการเมื่อได้รับข้อมูลไร้สาย (ESP-NOW Callback)
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    if (len == sizeof(SensorData)) {
        // ใช้ Semaphore เพื่อป้องกันการเขียนและอ่านข้อมูลพร้อมกัน (Thread Safety)
        if (data_mux != NULL && xSemaphoreTake(data_mux, pdMS_TO_TICKS(10))) {
            memcpy(&received_data, incomingData, sizeof(SensorData));
            xSemaphoreGive(data_mux);
        }
    } else {
        // แจ้งเตือนถ้าขนาดข้อมูลไม่ตรงกัน (อาจเกิดจากโครงสร้าง struct ในโค้ดทั้งสองฝั่งไม่เหมือนกัน)
        static unsigned long last_warn = 0;
        if(millis() - last_warn > 5000) {
            printf("คำเตือน: ขนาดข้อมูล ESP-NOW ไม่ตรงกัน! ได้รับ: %d, คาดหวัง: %d\n", len, (int)sizeof(SensorData));
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
    WiFi.disconnect();

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
        xSemaphoreGive(data_mux);
    }
    return current_data;
}
