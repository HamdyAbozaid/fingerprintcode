#pragma once
#include <ArduinoOTA.h>
#include "config.h"

class OTAManager {
public:
    static void setup();
    static void handle();
    static void checkForUpdates();
    
private:
    static void processUpdateResponse(String payload);
    static void startDownloadWithVerification();
};