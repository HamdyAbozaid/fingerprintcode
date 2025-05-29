#ifndef TIME_MODULE_H
#define TIME_MODULE_H

#include <Arduino.h>
#include <time.h> // For struct timeval

void timeSyncNotificationCallback(struct timeval *tv);
void setupNTP();
String getTimestamp();

#endif // TIME_MODULE_H
