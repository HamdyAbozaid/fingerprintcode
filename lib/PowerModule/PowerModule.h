#ifndef POWER_MODULE_H
#define POWER_MODULE_H

#include <Arduino.h>

void enterDeepSleep();
void powerDownPeripherals();
void manageLCDPower(); // Manages LCD backlight based on overall activity

#endif // POWER_MODULE_H
