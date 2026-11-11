#ifndef MEASUREMENT_H
#define MEASUREMENT_H
// ============================================================
//  measurement.h - Kiểu dữ liệu dùng chung giữa các module
// ============================================================

// Mức cảnh báo lũ
enum Level { NORMAL = 0, YELLOW = 1, RED = 2 };

// Toàn bộ số liệu của một chu kỳ đo (1 giây)
struct Measurement {
  // HC-SR04 / mực nước
  float dRaw        = -1;      // khoảng cách thô D_n (cm), -1 = mất echo
  float dLoc        = -1;      // khoảng cách đã lọc D_loc (cm), -1 = chưa có mẫu
  float hNuoc       = 0;       // mực nước H_nuoc = H_tram - D_loc (cm)
  Level level       = NORMAL;  // mức cảnh báo tính được trong chu kỳ này
  bool  filterReady = false;   // bộ lọc đã đủ N mẫu chưa

  // YF-S201
  float flowRate    = 0;       // Q (L/phút)
  float totalLitres = 0;       // tổng lít tích lũy

  // Độ ẩm đất
  int   soilRaw     = 0;       // ADC
  float soilPercent = 0;       // W (%)
};

#endif
