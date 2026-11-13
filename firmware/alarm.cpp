#include <Arduino.h>
#include "config.h"
#include "alarm.h"

static volatile int sNuoc = 0, sAm = 0, sLL = 0;   // mức từng cảm biến: 0 = BINH THUONG, 1 = VANG, 2 = DO

static void ledWrite(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

static void buzzerWrite(bool on) {
  bool mucCao = BUZZER_ACTIVE_LOW ? !on : on;
  digitalWrite(BUZZER_PIN, mucCao ? HIGH : LOW);
}

static int phanMuc(float v, float nguongVang, float nguongDo) {
  if (v >= nguongDo)   return 2;
  if (v >= nguongVang) return 1;
  return 0;
}

// Task chạy nền trên ESP32 để đèn + còi luôn nháy/kêu đều kể cả khi WiFi/HTTP đang bận
static void alarmTask(void* pvParameters) {
  for (;;) {
    alarmUpdate(millis());
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void alarmInit() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  ledWrite(false);
  buzzerWrite(false);

  // Tự kiểm tra lúc khởi động: đèn sáng và còi bíp 0,3 giây
  ledWrite(true);
  buzzerWrite(true);
  delay(300);
  ledWrite(false);
  buzzerWrite(false);

  xTaskCreate(alarmTask, "alarmTask", 2048, NULL, 1, NULL);
}

// Tính mức cảnh báo của từng cảm biến, gọi mỗi lần đo xong
void alarmEvaluate(const Measurement& m) {
  // Mực nước: nếu bộ lọc đã đủ mẫu thì dùng m.level (có trễ), nếu mới đo thì so ngưỡng trực tiếp
  if (m.filterReady) {
    sNuoc = (m.level == RED) ? 2 : ((m.level == YELLOW) ? 1 : 0);
  } else {
    sNuoc = (m.dLoc > 0) ? phanMuc(m.hNuoc, YELLOW_CM, RED_CM) : 0;
  }
  sAm = phanMuc(m.soilPercent, SOIL_VANG_PCT, SOIL_DO_PCT);
  sLL = phanMuc(m.flowRate,    FLOW_VANG_LPM, FLOW_DO_LPM);

  alarmUpdate(millis());   // cập nhật đèn + còi ngay lập tức
}

int alarmMucNuoc()     { return sNuoc; }
int alarmMucDoAm()     { return sAm; }
int alarmMucLuuLuong() { return sLL; }

// Mức chung = mức cao nhất trong 3 cảm biến
int alarmMucTong() {
  int t = sNuoc;
  if (sAm > t) t = sAm;
  if (sLL > t) t = sLL;
  return t;
}

const char* alarmNhanMuc(int muc) {
  return muc == 2 ? "DO" : (muc == 1 ? "VANG" : "BINH THUONG");
}

// Cảm biến gây ra mức cảnh báo cao nhất (ưu tiên: nước, độ ẩm, lưu lượng)
const char* alarmNguon() {
  int t = alarmMucTong();
  if (t == 0)     return "";
  if (sNuoc == t) return "Nuoc";
  if (sAm == t)   return "Do am";
  return "Luu luong";
}

// BINH THUONG: tắt hết
// VANG: đèn nháy + còi kêu ngắt quãng mỗi 1 giây (sáng/kêu 0,4 giây)
// DO:   đèn và còi nháy/kêu dồn dập (chu kỳ 0,4 giây)
void alarmUpdate(unsigned long now) {
  int muc = alarmMucTong();
  bool led = false, buz = false;

  if (muc == 2) {
    bool phase = (now % 400) < 200;
    led = phase;
    buz = phase;
  } else if (muc == 1) {
    bool phase = (now % 1000) < 400;
    led = phase;
    buz = phase;
  }

  ledWrite(led);
  buzzerWrite(buz);
}