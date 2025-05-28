#pragma once
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

class NetworkManager {
public:
    static bool autoConnect();
    static void startWiFiManager(bool forceConfig);
    static bool isConnected();
    static void setupNTP();
    static String getTimestamp();
};