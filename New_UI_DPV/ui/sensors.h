#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// Unified data structure shared between both boards
// โครงสร้างข้อมูลที่ใช้ร่วมกันระหว่างบอร์ดจอ (Receiver) และบอร์ดเซ็นเซอร์ (Transmitter)
// หมายเหตุ: โครงสร้างนี้ต้องเหมือนกันทั้งสองฝั่ง ห้ามแก้ไขจุดใดจุดหนึ่งเดี่ยวๆ มิฉะนั้นข้อมูลจะเพี้ยน
typedef struct {
    float heading;      // [BNO055] ทิศทางเข็มทิศ (0-360 องศา)
    float pitch;        // [BNO055] องศาการเงย/ทิ่ม (-180 ถึง 180) ยังไม่ได้ใช้
    float roll;         // [BNO055] องศาการเอียงซ้าย/ขวา (-90 ถึง 90) ยังไม่ได้ใช้
    float speed;        // [User] ค่าความเร็ว (km/h) ที่ส่งมาจากบอร์ดเซ็นเซอร์
    float depth;        // [Derived] ค่าความลึก (เมตร) คำนวณจากความดันที่บอร์ดเซ็นเซอร์
    float temperature;  // [BMP280] อุณหภูมิ (เซลเซียส) จากบอร์ดเซ็นเซอร์
    float pressure;     // [BMP280] ค่าความดันอากาศ (hPa)
    uint32_t packet_id; // [System] เลข ID ลำดับการส่งข้อมูล ใช้เช็คว่าข้อมูลที่ได้รับคือค่าใหม่ล่าสุด
    bool bno_online;    // สถานะการเชื่อมต่อของเซ็นเซอร์ BNO055
    bool bmp_online;    // สถานะการเชื่อมต่อของเซ็นเซอร์ BMP280
} SensorData;

// Function prototypes for the Main Board (Receiver)
void sensors_init();
void sensors_update();
SensorData sensors_get_data();

#endif
