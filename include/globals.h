#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Adafruit_Fingerprint.h>
#include <Bounce2.h>
#include <SPI.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_LittleFS.h>
#include "config.h" // Include config for pin numbers etc.

// LCD Instance
extern LiquidCrystal lcd;

// Fingerprint Sensor
extern HardwareSerial mySerial; // UART for fingerprint sensor
extern Adafruit_Fingerprint finger;

// External Flash & Filesystem Objects
extern SPIClass hspi; // HSPI bus
extern Adafruit_FlashTransport_SPI flashTransport;
extern Adafruit_SPIFlash flash;
extern Adafruit_LittleFS filesys;

// Power Management
extern RTC_DATA_ATTR int bootCount;
extern bool isOffline;
extern unsigned long lastActivity;
extern unsigned long lastSyncAttempt;

// OTA Status tracking
enum OTAState { OTA_IDLE, OTA_DOWNLOADING, OTA_VERIFYING, OTA_UPDATING };
extern OTAState otaState;
extern unsigned long otaStartTime;
extern String pendingFirmwareURL;
extern String expectedHash;

// Bounce for button debounce
extern Bounce debouncer;

// --- Function Prototypes (from main.cpp) ---
// These could also be moved to more specific module headers if further modularized
void setupOTA();
void checkForUpdates();
void processUpdateResponse(String payload);
void startDownloadWithVerification();
void enterRecoveryMode();
void displayOTAProgress(size_t progress, size_t total, const char* label);
void handleOTAError(ota_error_t error);
bool confirmUpdate(String newVersion, int firmwareSize);
void startSecureRecoveryUpdate(String updateInfo);
void initSystem();
bool isBatterySufficient();
String getDeviceID();
float getBatteryLevel();
bool autoConnectWiFi();
void startWiFiManager(bool forceConfig);
void verifyFingerprint();
void enterDeepSleep();
void powerDownPeripherals();
void manageLCDPower();
void recordLCDActivity();
void showFingerprintPrompt();
void showAttendanceResult(bool success, int userId);
void showOfflineWarning();
void logAttendanceOffline(int userId, const char* timestamp);
void syncOfflineLogs();
void timeSyncNotificationCallback(struct timeval *tv);
void setupNTP();
String getTimestamp();
bool sendToBackend(int userId, String timestamp);


#endif // GLOBALS_H