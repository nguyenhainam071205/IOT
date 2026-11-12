#ifndef WATER_LEVEL_H
#define WATER_LEVEL_H
// ============================================================
//  water_level.h - HC-SR04: đo khoảng cách, lọc trung bình động,
//                  tính mực nước và mức cảnh báo
// ============================================================
#include "measurement.h"

void  waterLevelInit();                  // cấu hình chân TRIG/ECHO
void  waterLevelUpdate(Measurement& m);  // đo 1 mẫu -> D_loc -> H_nuoc -> mức cảnh báo
Level waterLevelCurrent();               // mức cảnh báo đang có hiệu lực
void  waterLevelCommit(Level l);         // chốt mức mới (gọi sau khi đã gửi alert)

const char* levelName(Level l);          // "normal" | "yellow" | "red"  (dùng trong JSON)
const char* levelLabel(Level l);         // "BINH THUONG" | "VANG" | "DO" (dùng in Serial)

#endif
