#include "SystemModule.h"
#include "globals.h"        // For lcd, debouncer
#include "config.h"         // For BATTERY_MIN_VOLTAGE, BATTERY_PIN etc.
#include "DisplayModule.h"  // For recordLCDActivity, showFingerprintPrompt
#include "WiFiModule.h"     // For autoConnectWiFi, startWiFiManager
#include "TimeModule.h"     // For setupNTP
#include "OTAModule.h"      // For setupOTA, checkForUpdates
#include <WiFi.h>           // For WiFi.status()
#include <ESP.h>            // For ESP.getEfuseMac()
#include <sntp.h>           // For sntp_enabled()

void initSystem() {
  Serial.println("Initializing System...");
  lcd.clear();
  lcd.print("Initializing...");
  recordLCDActivity();

  if (!autoConnectWiFi()) {
    Serial.println("AutoConnect failed. Starting WiFiManager for configuration.");
    startWiFiManager(false); // Will restart ESP
  } else {
    setupNTP();
    setupOTA();
  }
}

bool isBatterySufficient() {
  float voltage = getBatteryLevel();
  Serial.printf("Battery Voltage: %.2fV\n", voltage);
  return voltage >= BATTERY_MIN_VOLTAGE;
}

String getDeviceID() {
  uint64_t chipid = ESP.getEfuseMac();
  char chipid_str[18];
  snprintf(chipid_str, sizeof(chipid_str), "ESP32-%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(chipid_str);
}

float getBatteryLevel() {
  int adcValue = analogRead(BATTERY_PIN);
  float voltage = adcValue * (3.3 / 4095.0) * 2.0; // Adjust multiplier for your voltage divider
  return voltage;
}

void enterRecoveryMode() {
  Serial.println("Entering Recovery Mode (Button held at boot)...");
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  lcd.clear();
  lcd.print("Recovery Mode");
  recordLCDActivity();
  lcd.setCursor(0, 1);
  lcd.print("Press:ChkUpd/WiFi");

  bool actionTaken = false;
  while(!actionTaken) {
    debouncer.update();
    if (debouncer.fell()) {
        if (debouncer.currentDuration() >= 2000) { // Long press for WiFi Manager
            Serial.println("Recovery Mode: WiFi Manager selected.");
            lcd.clear(); lcd.print("Recovery: WiFi Cfg"); delay(500);
            startWiFiManager(true); // This function will restart the ESP
            actionTaken = true; 
        } else { // Short press for "Check for Updates"
            Serial.println("Recovery Mode: Check for Updates selected.");
            lcd.clear(); lcd.print("Recovery: Chk FW"); delay(500);
            if (WiFi.status() != WL_CONNECTED) {
                if (!autoConnectWiFi()) {
                    Serial.println("Recovery: WiFi not connected for update check. Try WiFi Cfg option.");
                    lcd.clear(); lcd.print("No WiFi for Upd"); lcd.setCursor(0,1); lcd.print("Use WiFi Cfg opt"); delay(2000);
                    lcd.clear(); lcd.print("Recovery Mode"); lcd.setCursor(0,1); lcd.print("Press:ChkUpd/WiFi"); recordLCDActivity();
                    continue; 
                }
            }
            if (WiFi.status() == WL_CONNECTED) {
                if (!sntp_enabled()) setupNTP(); // Ensure NTP is running for HTTPS
                checkForUpdates(); 
            }
            actionTaken = true; 
            break; 
        }
    }
    yield();
  }
  if (!actionTaken) {
    Serial.println("Recovery mode: No action. Proceeding normal boot.");
    lcd.clear(); lcd.print("Exiting Recovery"); delay(1000);
  }
}
