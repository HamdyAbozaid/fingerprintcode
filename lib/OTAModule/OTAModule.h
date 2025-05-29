#ifndef OTA_MODULE_H
#define OTA_MODULE_H

#include <Arduino.h>
#include <ArduinoOTA.h> // For ota_error_t

void setupOTA();
void checkForUpdates();
void processUpdateResponse(String payload); // Kept here as it's part of OTA logic flow
void startDownloadWithVerification();    // Kept here
bool confirmUpdate(String newVersion, int firmwareSize); // User interaction for OTA
void handleOTAError(ota_error_t error);
void startSecureRecoveryUpdate(String updateInfo);


#endif // OTA_MODULE_H