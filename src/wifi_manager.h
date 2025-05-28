#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <esp_wifi.h>

void initWiFiManager();
void scanNetworks();
int getNetworkCount();
String getNetworkSSID(int index);
int getNetworkRSSI(int index);
int getNetworkEncryption(int index);
void sendDeauthPacket(String targetSSID, int targetIndex);

// ✅ THÊM: Continuous jamming control functions
bool isCurrentlyJamming();
String getJammingStatus();
void stopJammingAttack();

// Khai báo hàm từ main.cpp
extern void setupStealth();
extern int currentChannel;

#endif