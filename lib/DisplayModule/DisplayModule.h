#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <Arduino.h>

void recordLCDActivity();
void showFingerprintPrompt();
void showAttendanceResult(bool success, int userId);
void showOfflineWarning();
void displayOTAProgress(size_t progress, size_t total, const char* label); // Moved from OTAModule for direct LCD access

#endif // DISPLAY_MODULE_H