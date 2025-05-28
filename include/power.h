#pragma once
#include "config.h"

class PowerManager {
public:
    static void begin();
    static bool isRecoveryMode();
    static void enterRecoveryMode();
    static void manageSleep();
    static bool isBatterySufficient();
    static float getBatteryLevel();
};