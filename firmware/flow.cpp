#include <Arduino.h>
#include "config.h"
#include "flow.h"

// ---------------------- TRẠNG THÁI NỘI BỘ ----------------------
static volatile unsigned long pulseCount = 0;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static float totalLitres = 0.0;

// ---------------------- NGẮT ĐẾM XUNG ----------------------
static void IRAM_ATTR onPulse() {
  portENTER_CRITICAL_ISR(&mux);
  pulseCount = pulseCount + 1;   // mỗi cạnh xuống của YF-S201 = 1 xung
  portEXIT_CRITICAL_ISR(&mux);
}

// ---------------------- API ----------------------
void flowInit() {
  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), onPulse, FALLING);
}

// - Đếm số xung trong khoảng elapsed (~1 giây) -> tần số f (Hz)
// - f = 7.5 x Q  =>  Q = f / 7.5  (lít/phút)
// Chia cho elapsed thực tế thay vì giả định đúng 1000 ms, vì pulseIn()
// và HTTP POST có thể làm chu kỳ dài hơn 1 giây một chút.
void flowUpdate(unsigned long elapsedMs, Measurement& m) {
  portENTER_CRITICAL(&mux);
  unsigned long pulses = pulseCount;             // số xung đếm được trong chu kỳ
  pulseCount = 0;
  portEXIT_CRITICAL(&mux);

  float f = pulses * 1000.0 / elapsedMs;         // tần số xung (Hz) = xung / giây
  m.flowRate = f / FLOW_K;                       // Q (L/phút) = f / 7.5

  totalLitres += (m.flowRate / 60.0) * (elapsedMs / 1000.0);   // L/phút -> L/giây x giây
  m.totalLitres = totalLitres;
}
