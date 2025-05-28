#include "web_server.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <SPIFFS.h>

WebServer* webServer;

void startWebServer(WebServer& server) {
  webServer = &server;
  
  // Khởi tạo SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  // Routes cho static files
  server.on("/", handleRoot);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);
  
  // API routes
  server.on("/scan", handleScan);
  server.on("/networks", handleNetworks);
  server.on("/deauth", HTTP_POST, handleDeauth);
  
  // ✅ THÊM: Continuous jamming control routes
  server.on("/stop", handleStopJamming);
  server.on("/status", handleJammingStatus);
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    webServer->send(404, "text/plain", "File not found");
    return;
  }
  
  webServer->streamFile(file, "text/html");
  file.close();
}

void handleCSS() {
  File file = SPIFFS.open("/style.css", "r");
  if (!file) {
    webServer->send(404, "text/plain", "CSS file not found");
    return;
  }
  
  webServer->streamFile(file, "text/css");
  file.close();
}

void handleJS() {
  File file = SPIFFS.open("/script.js", "r");
  if (!file) {
    webServer->send(404, "text/plain", "JS file not found");
    return;
  }
  
  webServer->streamFile(file, "application/javascript");
  file.close();
}

void handleScan() {
  scanNetworks();
  webServer->send(200, "text/plain", "Scanning...");
}

void handleNetworks() {
  String json = "[";
  int count = getNetworkCount();
  
  for (int i = 0; i < count; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + getNetworkSSID(i) + "\",";
    json += "\"rssi\":" + String(getNetworkRSSI(i)) + ",";
    json += "\"encryption\":" + String(getNetworkEncryption(i));
    json += "}";
  }
  
  json += "]";
  webServer->send(200, "application/json", json);
}

void handleDeauth() {
  String body = webServer->arg("plain");
  Serial.println("Jamming request: " + body);
  
  // Check if already jamming
  if (isCurrentlyJamming()) {
    webServer->send(409, "text/plain", "Already jamming! Stop current attack first.");
    return;
  }
  
  // Parse JSON manually
  String ssid = "";
  int index = -1;
  
  // Tìm SSID
  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidStart);
  if (ssidStart > 7 && ssidEnd > ssidStart) {
    ssid = body.substring(ssidStart, ssidEnd);
  }
  
  // Tìm index
  int indexStart = body.indexOf("\"index\":") + 8;
  int indexEnd = body.indexOf("}", indexStart);
  if (indexStart > 7 && indexEnd > indexStart) {
    String indexStr = body.substring(indexStart, indexEnd);
    index = indexStr.toInt();
  }
  
  Serial.printf("Parsed - SSID: %s, Index: %d\n", ssid.c_str(), index);
  
  if (ssid.length() > 0 && index >= 0) {
    webServer->send(200, "text/plain", "Continuous jamming started on: " + ssid + " (Use /stop to end)");
    
    // Start jamming (runs in background)
    sendDeauthPacket(ssid, index);
  } else {
    webServer->send(400, "text/plain", "Invalid SSID or Index");
  }
}

// ✅ THÊM: Stop jamming endpoint
void handleStopJamming() {
  if (isCurrentlyJamming()) {
    stopJammingAttack();
    webServer->send(200, "text/plain", "Jamming stopped successfully");
    Serial.println("🛑 Jamming stopped via web interface");
  } else {
    webServer->send(200, "text/plain", "No active jamming to stop");
  }
}

// ✅ THÊM: Status endpoint
void handleJammingStatus() {
  String status = getJammingStatus();
  bool active = isCurrentlyJamming();
  
  String json = "{";
  json += "\"active\":" + String(active ? "true" : "false") + ",";
  json += "\"status\":\"" + status + "\"";
  json += "}";
  
  webServer->send(200, "application/json", json);
}