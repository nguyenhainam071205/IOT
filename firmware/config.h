#ifndef CONFIG_H
#define CONFIG_H
// ============================================================
//  config.h - Chân GPIO, WiFi/server và mọi hằng số cấu hình
// ============================================================

// ---------------------- CHÂN GPIO ----------------------
const int TRIG_PIN = 25;   // HC-SR04 TRIG
const int ECHO_PIN = 26;   // HC-SR04 ECHO (qua chia áp 1k/2k)
const int FLOW_PIN = 27;   // YF-S201 xung (qua chia áp 1k/2k)
const int SOIL_PIN = 34;   // Độ ẩm đất AO (ADC)

// ---------------------- WIFI / SERVER (tự điền) ----------------------
const char* const WIFI_SSID  = "@@@";
const char* const WIFI_PASS  = "Namdeptrai@@";
const char* const SERVER_URL = "http://10.146.124.224:3000/flood";

// ---------------------- HC-SR04 ----------------------
const float TEMP_C    = 30.0;   // nhiệt độ môi trường (°C) để bù tốc độ âm thanh
const float OFFSET_CM = 0.0;    // độ lệch cố định, chỉnh sau khi so với thước

// ---------------------- MỰC NƯỚC / CẢNH BÁO ----------------------
//   D_loc  = (1/N) * SUM(D[n-i], i = 0..N-1)   (trung bình động N mẫu)
//   H_nuoc = H_tram - D_loc
const float H_TRAM_CM = 30.0;   // chiều cao lắp cảm biến so với mốc chuẩn (đáy)
const int   N_MA      = 5;      // cửa sổ trung bình động (số mẫu)
const float YELLOW_CM = 4.0;    // H_nuoc >= 4 cm  -> cảnh báo VÀNG
const float RED_CM    = 8.0;    // H_nuoc >= 8 cm  -> cảnh báo ĐỎ
const float HYST_CM   = 0.5;    // trễ khi hạ mức, tránh nhấp nháy quanh ngưỡng

// ---------------------- YF-S201 (LƯU LƯỢNG) ----------------------
//   f = 7.5 x Q  =>  Q = f / 7.5   (f: Hz, Q: lít/phút)
const float FLOW_K = 7.5;       // số xung/giây ứng với 1 L/phút

// ---------------------- ĐỘ ẨM ĐẤT (LM393: khô -> ADC cao, ướt -> ADC thấp) ----------------------
//   W = (ADC_kho - ADC) / (ADC_kho - ADC_uot) x 100%
// Hai mốc hiệu chuẩn dưới đây là giá trị ví dụ, hãy đo lại bằng cảm biến của bạn:
const int ADC_KHO = 3500;       // ADC khi đất khô hoàn toàn (để ngoài không khí)
const int ADC_UOT = 1400;       // ADC khi đất ướt hoàn toàn (nhúng nước)

// ---------------------- CẢNH BÁO LƯU LƯỢNG / ĐỘ ẨM ĐẤT / LŨ QUÉT ----------------------
// Mốc là giá trị đề xuất ban đầu: hãy hiệu chuẩn theo lưu lượng bình thường tại chỗ đặt trạm.
// Cảnh báo lưu lượng dùng trung bình động FLOW_MA_N mẫu (1 mẫu/giây) cho đỡ nhảy.
//   Lũ quét ĐỎ   : lưu lượng >= vàng VÀ độ ẩm >= vàng cùng lúc
//   Lũ quét VÀNG : chỉ một trong hai đạt mức đỏ (nước chảy rất mạnh, hoặc đất bão hòa)
const int   FLOW_MA_N        = 5;      // cửa sổ trung bình động của lưu lượng (số mẫu)
const float FLOW_YELLOW_LPM  = 10.0;   // Q >= 10 L/phút -> lưu lượng VÀNG
const float FLOW_RED_LPM     = 20.0;   // Q >= 20 L/phút -> lưu lượng ĐỎ
const float FLOW_HYST_LPM    = 1.0;    // trễ khi hạ mức lưu lượng
const float SOIL_YELLOW_PCT  = 70.0;   // W >= 70 % -> đất ngấm nước (VÀNG)
const float SOIL_RED_PCT     = 90.0;   // W >= 90 % -> đất bão hòa (ĐỎ)
const float SOIL_HYST_PCT    = 3.0;    // trễ khi hạ mức độ ẩm

// ---------------------- CHU KỲ ----------------------
const unsigned long INTERVAL_MS        = 1000;                  // chu kỳ đo / in Serial
const unsigned long REPORT_INTERVAL_MS = 5UL * 1000UL;          // báo cáo định kỳ lên server: 5 giây
const unsigned long WIFI_RETRY_MS      = 10000;                 // thử kết nối lại WiFi mỗi 10 s

#endif
