#ifndef ALARM_H
#define ALARM_H

#include "measurement.h"

void alarmInit();                              // gọi 1 lần trong setup()
void alarmEvaluate(const Measurement& m);      // tính mức từng cảm biến, gọi mỗi lần đo
void alarmUpdate(unsigned long now);           // điều khiển đèn + còi, gọi liên tục trong loop()

// Mức: 0 = BINH THUONG, 1 = VANG, 2 = DO
int         alarmMucNuoc();
int         alarmMucDoAm();
int         alarmMucLuuLuong();
int         alarmMucTong();                    // mức cao nhất trong 3 cảm biến
const char* alarmNhanMuc(int muc);             // "BINH THUONG" / "VANG" / "DO"
const char* alarmNguon();                      // cảm biến gây cảnh báo cao nhất: "Nuoc" / "Do am" / "Luu luong"

#endif