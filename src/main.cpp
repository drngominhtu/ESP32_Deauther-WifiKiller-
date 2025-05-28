#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include "wifi_manager.h"
#include "web_server.h"

// Khai báo WebServer như một biến global
WebServer server(80);
int currentChannel = 6; // Lưu channel hiện tại

void setupStealth() {
  // 1. Random MAC
  uint8_t newMac[6];
  for(int i = 0; i < 6; i++) {
    newMac[i] = random(0, 255);
  }
  newMac[0] = (newMac[0] & 0xFE) | 0x02; // Set locally administered bit
  esp_wifi_set_mac(WIFI_IF_AP, &newMac[0]);
  
  // 2. Random IP
  uint8_t ip3 = random(1, 255);
  uint8_t ip4 = random(1, 255);
  IPAddress local_IP(192, 168, ip3, ip4);
  IPAddress gateway(192, 168, ip3, ip4);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  // 3. Sử dụng channel cố định để dễ khôi phục
  String ssid = "WiFi_" + String(random(10000, 99999));
  currentChannel = 6; // Channel phổ biến, dễ khôi phục
  
  WiFi.softAP(ssid.c_str(), "", currentChannel, false, 1); // No password, not hidden
  
  Serial.printf("Stealth Mode:\n");
  Serial.printf("SSID: %s\n", ssid.c_str());
  Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("Channel: %d\n", currentChannel);
}

// Hàm khôi phục AP sau khi deauth
void restoreAP() {
  delay(1000);
  WiFi.mode(WIFI_AP);
  setupStealth();
  Serial.println("AP restored after deauth attack");
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  
  Serial.println("🔒 Starting Advanced ESP32 Deauther...");
  Serial.println("⚡ Multi-technique deauth system");
  
  WiFi.mode(WIFI_AP);
  setupStealth();
  
  // Khởi tạo WiFi manager
  initWiFiManager();
  
  // Bắt đầu quét mạng WiFi
  scanNetworks();

  // Khởi động web server
  startWebServer(server);
  
  Serial.println("✅ Advanced deauther ready");
}

void loop() {
  server.handleClient();
  delay(10); // Thêm delay nhỏ để tránh watchdog reset
}