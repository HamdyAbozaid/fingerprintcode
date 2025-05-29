#ifndef SYSTEM_MODULE_H
#define SYSTEM_MODULE_H

#include <Arduino.h>

void initSystem();
bool isBatterySufficient();
String getDeviceID();
float getBatteryLevel();
void enterRecoveryMode(); // Moved here as it's a system-level boot action

#endif // SYSTEM_MODULE_H
