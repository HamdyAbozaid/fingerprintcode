#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>

bool autoConnectWiFi();
void startWiFiManager(bool forceConfig);

#endif // WIFI_MODULE_H