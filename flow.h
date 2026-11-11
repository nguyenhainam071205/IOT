#ifndef FLOW_H
#define FLOW_H
// ============================================================
//  flow.h - YF-S201: đếm xung bằng ngắt, tính lưu lượng Q = f / 7.5
// ============================================================
#include "measurement.h"

void flowInit();                                          // cấu hình chân + gắn ngắt
void flowUpdate(unsigned long elapsedMs, Measurement& m); // tính f, Q và tổng lít cho chu kỳ vừa qua

#endif
