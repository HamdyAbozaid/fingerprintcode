#ifndef STORAGE_MODULE_H
#define STORAGE_MODULE_H

#include <Arduino.h>

void logAttendanceOffline(int userId, const char* timestamp);
void syncOfflineLogs();

#endif // STORAGE_MODULE_H
