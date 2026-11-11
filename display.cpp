#include <Arduino.h>
#include "display.h"
#include "water_level.h"   // levelLabel()
#include "uplink.h"        // uplinkIsConnected()

void printStatus(const Measurement& m) {
  Serial.print("D_raw: ");
  if (m.dRaw < 0) Serial.print("--"); else Serial.print(m.dRaw, 1);
  Serial.print(" | D_loc: ");
  if (m.dLoc < 0) Serial.print("--"); else Serial.print(m.dLoc, 1);
  Serial.print(" cm | H_nuoc: ");
  Serial.print(m.hNuoc, 1);
  Serial.print(" cm | Muc: ");
  Serial.print(m.filterReady ? levelLabel(m.level) : "(dang khoi dong)");

  Serial.print(" | Luu luong: ");
  Serial.print(m.flowRate, 2);
  Serial.print(" L/phut (");
  Serial.print(m.totalLitres, 3);
  Serial.print(" L)");

  Serial.print(" | Do am dat: ");
  Serial.print(m.soilPercent, 1);
  Serial.print(" % (ADC ");
  Serial.print(m.soilRaw);
  Serial.print(")");

  Serial.print(" | WiFi: ");
  Serial.println(uplinkIsConnected() ? "OK" : "--");
}
