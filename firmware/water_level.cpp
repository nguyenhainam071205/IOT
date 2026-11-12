#include <Arduino.h>
#include "config.h"
#include "water_level.h"

// ---------------------- TRẠNG THÁI NỘI BỘ ----------------------
// Bộ lọc trung bình động: bộ đệm vòng + tổng chạy
static float maBuf[N_MA];
static int   maIdx   = 0;
static int   maCount = 0;
static float maSum   = 0.0;
static float dLoc    = -1;            // D_loc gần nhất (-1 = chưa có mẫu)

static Level currentLevel = NORMAL;   // mức cảnh báo đang có hiệu lực

// ---------------------- ĐO KHOẢNG CÁCH ----------------------
// Một lần đo thô, trả về cm hoặc -1 nếu không nhận được echo
static float readOnce() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  float speed = (331.3 + 0.606 * TEMP_C) / 10000.0;   // cm/µs
  return duration * speed / 2.0;
}

// Lấy một mẫu D_n cho chu kỳ này: thử tối đa 3 lần nếu mất echo
static float readSample() {
  for (int i = 0; i < 3; i++) {
    float d = readOnce();
    if (d > 0) return d + OFFSET_CM;
    delay(30);
  }
  return -1;
}

// ---------------------- BỘ LỌC TRUNG BÌNH ĐỘNG ----------------------
// Đưa mẫu mới vào cửa sổ N, trả về D_loc = trung bình các mẫu đang có.
// Dùng tổng chạy (running sum) nên mỗi mẫu chỉ tốn O(1).
static float movingAverage(float d) {
  if (maCount == N_MA) maSum -= maBuf[maIdx];   // bỏ mẫu cũ nhất ra khỏi tổng
  maBuf[maIdx] = d;
  maSum += d;
  maIdx = (maIdx + 1) % N_MA;
  if (maCount < N_MA) maCount++;
  return maSum / maCount;
}

// ---------------------- MỨC CẢNH BÁO ----------------------
// Lên mức: so ngưỡng thẳng. Hạ mức: phải tụt dưới (ngưỡng - HYST_CM) mới hạ,
// tránh gửi cảnh báo liên tục khi mực nước dao động quanh ngưỡng.
static Level computeLevel(float h, Level prev) {
  if (h >= RED_CM)                                 return RED;
  if (prev == RED    && h >= RED_CM    - HYST_CM)  return RED;
  if (h >= YELLOW_CM)                              return YELLOW;
  if (prev == YELLOW && h >= YELLOW_CM - HYST_CM)  return YELLOW;
  return NORMAL;
}

const char* levelName(Level l) {
  switch (l) {
    case RED:    return "red";
    case YELLOW: return "yellow";
    default:     return "normal";
  }
}

const char* levelLabel(Level l) {
  switch (l) {
    case RED:    return "DO";
    case YELLOW: return "VANG";
    default:     return "BINH THUONG";
  }
}

// ---------------------- API ----------------------
void waterLevelInit() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
}

void waterLevelUpdate(Measurement& m) {
  m.dRaw = readSample();
  if (m.dRaw > 0) dLoc = movingAverage(m.dRaw);   // mất echo: giữ D_loc cũ
  m.dLoc = dLoc;

  m.filterReady = (maCount == N_MA);              // bộ lọc đã đủ N mẫu

  m.hNuoc = (dLoc > 0) ? (H_TRAM_CM - dLoc) : 0;  // H_nuoc = H_tram - D_loc
  if (m.hNuoc < 0) m.hNuoc = 0;                   // nước thấp hơn mốc chuẩn -> coi như 0

  // Chỉ xét cảnh báo khi bộ lọc đã đầy, tránh cảnh báo giả lúc mới bật
  m.level = m.filterReady ? computeLevel(m.hNuoc, currentLevel) : currentLevel;
}

Level waterLevelCurrent() {
  return currentLevel;
}

void waterLevelCommit(Level l) {
  currentLevel = l;
}
