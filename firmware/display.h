#ifndef DISPLAY_H
#define DISPLAY_H
// ============================================================
//  display.h - In số liệu ra Serial Monitor và hiển thị lên LCD 1602 I2C
// ============================================================
#include "measurement.h"

void displayInit();                       // khởi tạo LCD, gọi 1 lần trong setup()
void printStatus(const Measurement& m);   // in Serial và cập nhật LCD

#endif