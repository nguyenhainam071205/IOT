#ifndef UPLINK_H
#define UPLINK_H
// ============================================================
//  uplink.h - WiFi và gửi HTTP POST JSON lên web server
//  (không đặt tên network.h vì trùng Network.h của core ESP32)
// ============================================================
#include "measurement.h"

void uplinkInit();                       // kết nối WiFi lần đầu (chờ tối đa 15 s, không treo)
void uplinkMaintain(unsigned long now);  // tự kết nối lại nếu rớt, không chặn
bool uplinkIsConnected();

// event: "alert" (đổi mức cảnh báo) hoặc "report" (định kỳ)
bool sendToServer(const char* event, const Measurement& m);

#endif
