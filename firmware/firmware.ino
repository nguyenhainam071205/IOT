/*
  ESP32: Trạm đo lũ - HC-SR04 (mực nước) + YF-S201 (lưu lượng) + độ ẩm đất (LM393)
         + LCD 1602 I2C hiển thị + đèn LED và còi báo động

  - Đo mỗi giây, in 1 dòng ra Serial Monitor (115200 baud) và hiển thị lên LCD
  - Mực nước: D lọc trung bình động N mẫu, H_nuoc = H_tram - D_loc
  - Cảnh báo mực nước: H_nuoc >= 4 cm -> VÀNG, H_nuoc >= 8 cm -> ĐỎ
  - Cảnh báo độ ẩm đất và lưu lượng theo ngưỡng trong config.h
  - Mức cảnh báo chung = mức cao nhất trong 3 cảm biến
  - Đèn LED và còi: VÀNG nháy chậm + bíp ngắn, ĐỎ nháy nhanh
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
    display.*       - in Serial + hiển thị LCD 1602 I2C
    alarm.*         - cảnh báo 3 cảm biến + đèn LED + còi báo động

  Đấu nối:
  - HC-SR04  : VCC -> VIN(5V), GND -> GND, TRIG -> GPIO 25, ECHO -> chia áp 1k/2k -> GPIO 26
  - YF-S201  : đỏ -> VIN(5V), đen -> GND, vàng -> chia áp 1k/2k -> GPIO 27
  - Độ ẩm đất: VCC -> 3V3, GND -> GND, AO -> GPIO 34
  - LCD 1602 I2C: VCC -> VIN(5V), GND -> GND, SDA -> GPIO 21, SCL -> GPIO 22
  - Module LED: VCC -> 5V, GND -> GND, IN -> GPIO 19
  - Module còi : VCC -> 3V3, GND -> GND, I/O -> GPIO 18
*/

#include "config.h"
#include "measurement.h"
#include "water_level.h"
#include "flow.h"
#include "soil.h"
#include "alert.h"
#include "uplink.h"
#include "display.h"
#include "alarm.h"

// Chỉ loop() dùng các biến này
unsigned long lastTime     = 0;
unsigned long lastReport   = 0;
bool          reportedOnce = false;

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(500);

  displayInit();        // khởi tạo LCD 1602 I2C
  alarmInit();          // khởi tạo đèn + còi (tự bíp thử 0,3 giây)
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
  alarmUpdate(now);           // SỬA: chỉ truyền now, phải đặt trước dòng return bên dưới

  unsigned long elapsed = now - lastTime;
  if (elapsed < INTERVAL_MS) return;
  lastTime = now;

  Measurement m;
  flowUpdate(elapsed, m);     // 1) lưu lượng
  waterLevelUpdate(m);        // 2) khoảng cách -> D_loc -> H_nuoc -> mức cảnh báo
  soilUpdate(m);              // 3) độ ẩm đất
  alertUpdate(m);             // mức lưu lượng, độ ẩm, lũ quét, mức tổng hợp
  alarmEvaluate(m);           // SỬA: thêm dòng này, tính mức cảnh báo của 3 cảm biến
  printStatus(m);             // 4) in Serial + cập nhật LCD

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