#ifndef CONFIG_H
#define CONFIG_H
// ============================================================
//  config.h - Chân GPIO, WiFi/server và mọi hằng số cấu hình
// ============================================================
#include <Arduino.h>

// ---------------------- CHÂN GPIO ----------------------
const int TRIG_PIN = 25;   // HC-SR04 TRIG
const int ECHO_PIN = 26;   // HC-SR04 ECHO (qua chia áp 1k/2k)
const int FLOW_PIN = 27;   // YF-S201 xung (qua chia áp 1k/2k)
const int SOIL_PIN = 34;   // Độ ẩm đất AO (ADC)

// ---------------------- LCD 1602 I2C ----------------------
const int     LCD_SDA  = 21;     // SDA -> D21
const int     LCD_SCL  = 22;     // SCL -> D22
const uint8_t LCD_ADDR = 0x27;   // nếu không lên chữ, đổi thành 0x3F

// ---------------------- ĐÈN + CÒI BÁO ĐỘNG ----------------------
const int  LED_PIN           = 19;     // module LED: IN -> D19
const int  BUZZER_PIN        = 18;     // module còi: I/O -> D18
const bool BUZZER_ACTIVE_LOW = true;   // MH-FMD thường kích mức thấp; nếu còi kêu ngược thì đổi thành false

// ---------------------- WIFI / SERVER (tự điền) ----------------------
const char* const WIFI_SSID  = "GIAHUNG-305";
const char* const WIFI_PASS  = "GIAHUNG1368";
const char* const SERVER_URL = "http://192.168.35.101:3000/flood";

// ---------------------- HC-SR04 ----------------------
const float TEMP_C    = 30.0;   // nhiệt độ môi trường (°C) để bù tốc độ âm thanh
const float OFFSET_CM = 0.0;    // độ lệch cố định, chỉnh sau khi so với thước

// ---------------------- MỰC NƯỚC / CẢNH BÁO ----------------------
//  D_loc = (1/N) * SUM(D[n-i], i = 0..N-1)   (trung bình động N mẫu)
const int   N_MA      = 5;      // số mẫu của bộ lọc trung bình động
const float H_TRAM_CM = 10.0;   // chiều cao từ cảm biến xuống đáy (cm), H_nuoc = H_tram - D_loc
const float YELLOW_CM = 4.0;    // H_nuoc >= 4 cm  -> VÀNG
const float RED_CM    = 7.0;    // H_nuoc >= 7 cm  -> ĐỎ
const float HYST_CM   = 0.5;    // trễ khi hạ mức, tránh nhấp nháy giữa 2 mức

// ---------------------- LƯU LƯỢNG YF-S201 ----------------------
const float FLOW_K = 7.5;       // Q (L/phút) = f / 7.5

// ---------------------- ĐỘ ẨM ĐẤT ----------------------
const int ADC_KHO = 4095;       // ADC khi đất khô (0 %)
const int ADC_UOT = 1500;       // ADC khi đất ướt (100 %), cần hiệu chuẩn lại

// ---------------------- NGƯỠNG CẢNH BÁO ĐỘ ẨM ĐẤT + LƯU LƯỢNG ----------------------
const float SOIL_VANG_PCT = 70.0;   // độ ẩm đất >= 70 %      -> VÀNG
const float SOIL_DO_PCT   = 90.0;   // độ ẩm đất >= 90 %      -> ĐỎ
const float FLOW_VANG_LPM = 10.0;   // lưu lượng >= 10 L/phút -> VÀNG
const float FLOW_DO_LPM   = 20.0;   // lưu lượng >= 20 L/phút -> ĐỎ

// ---------------------- THỜI GIAN ----------------------
const unsigned long INTERVAL_MS        = 1000;    // chu kỳ đo: 1 giây
const unsigned long REPORT_INTERVAL_MS = 5000;    // chu kỳ gửi báo cáo: 5 giây
const unsigned long WIFI_RETRY_MS      = 10000;   // thử nối lại WiFi mỗi 10 giây

#endif