#pragma once
#include <Adafruit_Fingerprint.h>
#include "config.h"

class FingerprintManager {
public:
    static void begin();
    static void verifyFingerprint();
    
private:
    static void showFingerprintPrompt();
    static void showAttendanceResult(bool success, int userId);
};