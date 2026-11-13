#include <Arduino.h>
#include "config.h"
#include "alert.h"
#include "water_level.h"   // waterLevelCurrent()

// ---------------------- TRẠNG THÁI NỘI BỘ ----------------------
// Trung bình động của lưu lượng: bộ đệm vòng + tổng chạy (giống bộ lọc mực nước)
static float flowBuf[FLOW_MA_N];
static int   flowIdx   = 0;
static int   flowCount = 0;
static float flowSum   = 0.0;

// Các mức đã chốt (có hiệu lực), dùng làm "mức trước" cho độ trễ
static Level committedFlow    = NORMAL;
static Level committedSoil    = NORMAL;
static Level committedFlash   = NORMAL;
static Level committedOverall = NORMAL;

// ---------------------- MỨC THEO NGƯỠNG ----------------------
// Lên mức: so ngưỡng thẳng. Hạ mức: phải tụt dưới (ngưỡng - hyst) mới hạ.
Level levelFromValue(float v, float yellow, float red, float hyst, Level prev) {
  if (v >= red)                              return RED;
  if (prev == RED    && v >= red    - hyst)  return RED;
  if (v >= yellow)                           return YELLOW;
  if (prev == YELLOW && v >= yellow - hyst)  return YELLOW;
  return NORMAL;
}

// ---------------------- LŨ QUÉT ----------------------
Level flashFloodLevel(Level flow, Level soil) {
  if (flow >= YELLOW && soil >= YELLOW) return RED;      // nước chảy mạnh trên nền đất đã no nước
  if (flow == RED || soil == RED)       return YELLOW;   // một yếu tố đã ở mức nguy hiểm
  return NORMAL;
}

// ---------------------- API ----------------------
void alertUpdate(Measurement& m) {
  // Trung bình động Q: bỏ mẫu cũ nhất ra khỏi tổng khi cửa sổ đã đầy
  if (flowCount == FLOW_MA_N) flowSum -= flowBuf[flowIdx];
  flowBuf[flowIdx] = m.flowRate;
  flowSum += m.flowRate;
  flowIdx = (flowIdx + 1) % FLOW_MA_N;
  if (flowCount < FLOW_MA_N) flowCount++;
  m.flowAvg = flowSum / flowCount;

  // Chỉ xét cảnh báo khi bộ lọc mực nước đã đầy (tránh cảnh báo giả lúc mới bật)
  if (m.filterReady) {
    m.flowLevel = levelFromValue(m.flowAvg,     FLOW_YELLOW_LPM, FLOW_RED_LPM, FLOW_HYST_LPM, committedFlow);
    m.soilLevel = levelFromValue(m.soilPercent, SOIL_YELLOW_PCT, SOIL_RED_PCT, SOIL_HYST_PCT, committedSoil);
  } else {
    m.flowLevel = committedFlow;
    m.soilLevel = committedSoil;
  }
  m.flashLevel = flashFloodLevel(m.flowLevel, m.soilLevel);
  m.level = (m.waterLevel > m.flashLevel) ? m.waterLevel : m.flashLevel;
}

bool alertChanged(const Measurement& m) {
  return m.waterLevel != waterLevelCurrent()
      || m.flowLevel  != committedFlow
      || m.soilLevel  != committedSoil
      || m.flashLevel != committedFlash;
}

Level alertCommittedOverall() {
  return committedOverall;
}

void alertCommit(const Measurement& m) {
  committedFlow    = m.flowLevel;
  committedSoil    = m.soilLevel;
  committedFlash   = m.flashLevel;
  committedOverall = m.level;
}
