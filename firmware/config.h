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
const char* const WIFI_SSID  = "PTIT_WIFI";
const char* const WIFI_PASS  = "b23dcce087";
const char* const SERVER_URL = "http://10.146.124.224:3000/flood";

// ---------------------- HC-SR04 ----------------------
const float TEMP_C    = 30.0;   // nhiệt độ môi trường (°C) để bù tốc độ âm thanh
const float OFFSET_CM = 0.0;    // độ lệch cố định, chỉnh sau khi so với thước

// ---------------------- MỰC NƯỚC / CẢNH BÁO ----------------------
//  D_loc = (1/N) * SUM(D[n-i], i = 0..N-1)   (trung bình động N mẫu)
const int   N_MA      = 5;      // số mẫu của bộ lọc trung bình động
const float H_TRAM_CM = 30.0;   // chiều cao từ cảm biến xuống đáy (cm), H_nuoc = H_tram - D_loc
const float YELLOW_CM = 4.0;    // H_nuoc >= 4 cm  -> VÀNG
const float RED_CM    = 8.0;    // H_nuoc >= 8 cm  -> ĐỎ
const float HYST_CM   = 0.5;    // trễ khi hạ mức, tránh nhấp nháy giữa 2 mức

// ---------------------- LƯU LƯỢNG YF-S201 ----------------------
const float FLOW_K = 7.5;       // Q (L/phút) = f / 7.5

// ---------------------- ĐỘ ẨM ĐẤT ----------------------
const int ADC_KHO = 4095;       // ADC khi đất khô (0 %)
const int ADC_UOT = 1500;       // ADC khi đất ướt (100 %), cần hiệu chuẩn lại

// ---------------------- NGƯỠNG CẢNH BÁO ĐỘ ẨM ĐẤT + LƯU LƯỢNG ----------------------
const float SOIL_YELLOW_PCT = 70.0;   // độ ẩm đất >= 70 %      -> VÀNG
const float SOIL_RED_PCT    = 90.0;   // độ ẩm đất >= 90 %      -> ĐỎ
const float SOIL_HYST_PCT   = 2.0;    // trễ khi hạ mức độ ẩm (%)
const float FLOW_YELLOW_LPM = 10.0;   // lưu lượng >= 10 L/phút -> VÀNG
const float FLOW_RED_LPM    = 20.0;   // lưu lượng >= 20 L/phút -> ĐỎ
const float FLOW_HYST_LPM   = 1.0;    // trễ khi hạ mức lưu lượng (L/phút)
const int   FLOW_MA_N       = 5;      // số mẫu trung bình động của lưu lượng

// ---------------------- THỜI GIAN ----------------------
const unsigned long INTERVAL_MS        = 1000;    // chu kỳ đo: 1 giây
const unsigned long REPORT_INTERVAL_MS = 5000;    // chu kỳ gửi báo cáo: 5 giây
const unsigned long WIFI_RETRY_MS      = 10000;   // thử nối lại WiFi mỗi 10 giây

#endif