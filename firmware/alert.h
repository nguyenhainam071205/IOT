#ifndef ALERT_H
#define ALERT_H
// ============================================================
//  alert.h - Mức cảnh báo lưu lượng, độ ẩm đất, lũ quét và mức tổng hợp
// ============================================================
#include "measurement.h"

// Mức theo một ngưỡng vàng/đỏ, có độ trễ khi hạ mức (prev = mức đang có hiệu lực).
// Dùng chung cho mực nước, lưu lượng và độ ẩm đất.
Level levelFromValue(float v, float yellow, float red, float hyst, Level prev);

// Lũ quét: ĐỎ khi lưu lượng và độ ẩm cùng >= vàng; VÀNG khi chỉ một trong hai đạt đỏ.
Level flashFloodLevel(Level flow, Level soil);

// Gọi mỗi chu kỳ sau waterLevelUpdate/flowUpdate/soilUpdate: tính flowAvg,
// flowLevel, soilLevel, flashLevel và mức tổng hợp m.level.
void alertUpdate(Measurement& m);

bool  alertChanged(const Measurement& m);   // có mức nào khác mức đã chốt không (nước, Q, W, lũ quét)
Level alertCommittedOverall();              // mức tổng hợp đã chốt gần nhất
void  alertCommit(const Measurement& m);    // chốt các mức mới (gọi sau khi đã gửi alert)

#endif
