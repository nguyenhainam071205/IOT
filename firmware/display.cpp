#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "measurement.h"
#include "display.h"
#include "water_level.h"   // levelLabel()
#include "uplink.h"        // uplinkIsConnected()
#include "alarm.h"         // alarmMucTong(), alarmNhanMuc(), alarmNguon()

static LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// Khởi tạo LCD, gọi 1 lần trong setup()
void displayInit() {
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Khoi dong...");
}

// Luân phiên mỗi 3 giây:
//   Trang 1: Nuoc (mực nước %), Am (độ ẩm đất %), Luu luong (L/phút)
//   Trang 2: mức cảnh báo chung (cao nhất trong 3 cảm biến) và cảm biến gây ra
static void lcdShow(const Measurement& m) {
  // Mực nước theo % ngưỡng ĐỎ (100% = chạm ngưỡng ĐỎ), độ ẩm đất theo %
  float nuocPct = constrain(m.hNuoc / RED_CM * 100.0f, 0.0f, 100.0f);
  float amPct   = constrain(m.soilPercent, 0.0f, 100.0f);

  char l1[17], l2[17], tmp[24];
  bool trangCanhBao = ((millis() / 3000) % 2) == 1;   // đổi trang mỗi 3 giây

  if (!trangCanhBao) {
    // Trang 1: mực nước, độ ẩm, lưu lượng
    snprintf(tmp, sizeof(tmp), "Nuoc%3.0f%% Am%3.0f%%", nuocPct, amPct);
    snprintf(l1, sizeof(l1), "%-16.16s", tmp);
    snprintf(tmp, sizeof(tmp), "Luu luong %.1fL/p", m.flowRate);
    snprintf(l2, sizeof(l2), "%-16.16s", tmp);
  } else {
    // Trang 2: mức cảnh báo chung và cảm biến gây ra
    int tong = alarmMucTong();
    if (tong == 0) {
      snprintf(l1, sizeof(l1), "%-16.16s", "Muc canh bao:");
      snprintf(l2, sizeof(l2), "%-16.16s", m.filterReady ? "BINH THUONG" : "Khoi dong");
    } else {
      snprintf(l1, sizeof(l1), "%-16.16s", "!! CANH BAO !!");
      snprintf(tmp, sizeof(tmp), "%s: %s", alarmNhanMuc(tong), alarmNguon());
      snprintf(l2, sizeof(l2), "%-16.16s", tmp);
    }
  }

  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

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

  Serial.print(" | Canh bao chung: ");
  Serial.print(alarmNhanMuc(alarmMucTong()));
  if (alarmMucTong() > 0) {
    Serial.print(" (");
    Serial.print(alarmNguon());
    Serial.print(")");
  }

  Serial.print(" | WiFi: ");
  Serial.println(uplinkIsConnected() ? "OK" : "--");

  lcdShow(m);   // cập nhật LCD cùng lúc với Serial
}