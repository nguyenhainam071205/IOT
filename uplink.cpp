#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "uplink.h"
#include "water_level.h"   // levelName()

static unsigned long lastWifiRetry = 0;

// ---------------------- WIFI ----------------------
void uplinkInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Ket noi WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" OK, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" that bai, se thu lai trong loop()");
  }
}

void uplinkMaintain(unsigned long now) {
  if (WiFi.status() != WL_CONNECTED && now - lastWifiRetry >= WIFI_RETRY_MS) {
    lastWifiRetry = now;
    Serial.println("WiFi mat ket noi, thu lai...");
    WiFi.reconnect();
  }
}

bool uplinkIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

// ---------------------- HTTP POST JSON ----------------------
bool sendToServer(const char* event, const Measurement& m) {
  if (!uplinkIsConnected()) {
    Serial.printf("POST %s bo qua: chua co WiFi\n", event);
    return false;
  }

  String body = String("{\"event\":\"") + event +
                "\",\"level\":\"" + levelName(m.level) +
                "\",\"h_nuoc_cm\":" + String(m.hNuoc, 1) +
                ",\"d_loc_cm\":" + String(m.dLoc, 1) +
                ",\"h_tram_cm\":" + String(H_TRAM_CM, 1) +
                ",\"flow_lpm\":" + String(m.flowRate, 2) +
                ",\"total_l\":" + String(m.totalLitres, 3) +
                ",\"soil_pct\":" + String(m.soilPercent, 1) + "}";

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  int code = http.POST(body);
  http.end();

  Serial.printf("POST %s -> %d\n", event, code);
  return code > 0 && code < 300;
}
