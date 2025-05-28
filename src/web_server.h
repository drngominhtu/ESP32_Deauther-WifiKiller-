#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>

void startWebServer(WebServer& server);
void handleRoot();
void handleCSS();
void handleJS();
void handleScan();
void handleNetworks();
void handleDeauth();

// ✅ THÊM: Khai báo các hàm mới
void handleStopJamming();
void handleJammingStatus();

#endif