/*
  ESP32: Trạm đo lũ - HC-SR04 (mực nước) + YF-S201 (lưu lượng) + độ ẩm đất (LM393)

  - Đo mỗi giây, in 1 dòng ra Serial Monitor (115200 baud)
  - Mực nước: D lọc trung bình động N mẫu, H_nuoc = H_tram - D_loc
  - Cảnh báo mực nước: H_nuoc >= 4 cm -> VÀNG, H_nuoc >= 8 cm -> ĐỎ
  - Cảnh báo lưu lượng / độ ẩm đất / lũ quét: mốc trong config.h, logic trong alert.cpp
  - Gửi HTTP POST JSON về web server khi đổi mức cảnh báo và định kỳ 5 giây/lần

  Mở file này trong Arduino IDE. Thư mục chứa nó phải tên là "firmware"
  (trùng tên file firmware.ino). Code web server/giao diện nằm riêng ở ../web.

  Cấu trúc file (Arduino IDE tự mở thành các tab, nạp từ tab này là nạp hết):
    config.h        - chân GPIO, WiFi/server, mọi hằng số
    measurement.h   - enum Level, struct Measurement
    water_level.*   - HC-SR04, bộ lọc trung bình động, mực nước, mức cảnh báo
    flow.*          - YF-S201, ngắt đếm xung, lưu lượng
    soil.*          - độ ẩm đất
    alert.*         - mức cảnh báo lưu lượng, độ ẩm đất, lũ quét, mức tổng hợp
    uplink.*        - WiFi + HTTP POST JSON lên server
    display.*       - in Serial

  Đấu nối:
  - HC-SR04  : VCC -> VIN(5V), GND -> GND, TRIG -> GPIO 25, ECHO -> chia áp 1k/2k -> GPIO 26
  - YF-S201  : đỏ -> VIN(5V), đen -> GND, vàng -> chia áp 1k/2k -> GPIO 27
  - Độ ẩm đất: VCC -> 3V3, GND -> GND, AO -> GPIO 34
*/

#include "config.h"
#include "measurement.h"
#include "water_level.h"
#include "flow.h"
#include "soil.h"
#include "alert.h"
#include "uplink.h"
#include "display.h"

// Chỉ loop() dùng các biến này
unsigned long lastTime     = 0;
unsigned long lastReport   = 0;
bool          reportedOnce = false;

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(500);

  waterLevelInit();
  flowInit();
  uplinkInit();

  lastTime = millis();
  Serial.println("San sang: HC-SR04 + YF-S201 + do am dat");
}

// ===================== LOOP =====================
void loop() {
  unsigned long now = millis();
  uplinkMaintain(now);

  unsigned long elapsed = now - lastTime;
  if (elapsed < INTERVAL_MS) return;
  lastTime = now;

  Measurement m;
  flowUpdate(elapsed, m);     // 1) lưu lượng
  waterLevelUpdate(m);        // 2) khoảng cách -> D_loc -> H_nuoc -> mức cảnh báo
  soilUpdate(m);              // 3) độ ẩm đất
  alertUpdate(m);             // 4) Q trung bình, mức Q / W / lũ quét, mức tổng hợp
  printStatus(m);             // 5) in Serial

  if (!m.filterReady) return; // chưa đủ N mẫu: chưa cảnh báo, chưa gửi

  // 6) Gửi ngay khi bất kỳ mức nào đổi (mực nước, lưu lượng, độ ẩm, lũ quét; kể cả hạ mức)
  if (alertChanged(m)) {
    Serial.printf("Doi muc canh bao: %s -> %s (nuoc=%s, Q=%s, W=%s, lu quet=%s)\n",
                  levelLabel(alertCommittedOverall()), levelLabel(m.level),
                  levelLabel(m.waterLevel), levelLabel(m.flowLevel),
                  levelLabel(m.soilLevel), levelLabel(m.flashLevel));
    waterLevelCommit(m.waterLevel);
    alertCommit(m);
    sendToServer("alert", m);
  }

  // 7) Báo cáo định kỳ (lần đầu ngay khi bộ lọc đầy)
  if (!reportedOnce || now - lastReport >= REPORT_INTERVAL_MS) {
    reportedOnce = true;
    lastReport   = now;
    sendToServer("report", m);
  }
}
