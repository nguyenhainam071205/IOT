#include <Arduino.h>
#include "config.h"
#include "soil.h"

// Đọc ADC 10 lần rồi lấy trung bình để giảm nhiễu
static int readSoilRaw() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(SOIL_PIN);
    delay(2);
  }
  return sum / 10;
}

// Quy đổi ADC -> độ ẩm đất W (%):
//   W = (ADC_kho - ADC) / (ADC_kho - ADC_uot) x 100
// Kết quả giới hạn trong [0, 100] vì ADC thực tế có thể vượt ra ngoài hai mốc hiệu chuẩn.
static float soilPercentFromAdc(int adc) {
  float w = (float)(ADC_KHO - adc) / (float)(ADC_KHO - ADC_UOT) * 100.0;
  if (w < 0)   w = 0;
  if (w > 100) w = 100;
  return w;
}

// ---------------------- API ----------------------
void soilUpdate(Measurement& m) {
  m.soilRaw     = readSoilRaw();
  m.soilPercent = soilPercentFromAdc(m.soilRaw);
}
