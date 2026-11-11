#ifndef SOIL_H
#define SOIL_H
// ============================================================
//  soil.h - Độ ẩm đất: đọc ADC và quy đổi ra W (%)
// ============================================================
#include "measurement.h"

void soilUpdate(Measurement& m);   // đọc ADC (trung bình 10 lần) -> W (%)

#endif
