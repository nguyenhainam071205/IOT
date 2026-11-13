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
  Serial.print(m.filterReady ? levelLabel(m.waterLevel) : "(dang khoi dong)");

  Serial.print(" | Luu luong: ");
  Serial.print(m.flowRate, 2);
  Serial.print(" L/phut (TB ");
  Serial.print(m.flowAvg, 2);
  Serial.print(", ");
  Serial.print(m.totalLitres, 3);
  Serial.print(" L) ");
  Serial.print(levelLabel(m.flowLevel));

  Serial.print(" | Do am dat: ");
  Serial.print(m.soilPercent, 1);
  Serial.print(" % (ADC ");
  Serial.print(m.soilRaw);
  Serial.print(") ");
  Serial.print(levelLabel(m.soilLevel));

  Serial.print(" | Lu quet: ");
  Serial.print(levelLabel(m.flashLevel));

  Serial.print(" | WiFi: ");
  Serial.println(uplinkIsConnected() ? "OK" : "--");
}
