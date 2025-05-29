#include "TimeModule.h"
#include "globals.h"       // For lcd
#include "config.h"        // For NTP_SERVER1, GMT_OFFSET_SEC etc.
#include "DisplayModule.h" // For showFingerprintPrompt
#include <sntp.h>          // For sntp_set_time_sync_notification_cb, sntp_enabled

void timeSyncNotificationCallback(struct timeval *tv) {
  Serial.printf("NTP time synchronized. Current time (UTC): %ld.%06ld\n", tv->tv_sec, tv->tv_usec);
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){ // getLocalTime uses the configured offsets
    Serial.printf("Current local time: %s", asctime(&timeinfo));
    lcd.clear();
    lcd.print("Time Synced!");
    lcd.setCursor(0, 1);
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &timeinfo);
    lcd.print(time_buf);
    delay(1500);
    showFingerprintPrompt();
  }
}

void setupNTP() {
  Serial.println("Setting up NTP...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
  sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) { // Wait up to 10s
    Serial.println("Failed to obtain time (initial check). NTP client will retry.");
    lcd.clear();
    lcd.print("Time Sync Fail");
    delay(1500);
    showFingerprintPrompt();
  } else {
    Serial.println("Initial time fetch successful.");
    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &timeinfo);
    lcd.clear();
    lcd.print("Time OK:");
    lcd.setCursor(0,1);
    lcd.print(time_buf);
    delay(1500);
    showFingerprintPrompt();
  }
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time for timestamp. Using millis-based fallback.");
    unsigned long secondsSinceBoot = millis() / 1000;
    return "NO_NTP_TIME_" + String(secondsSinceBoot);
  }
  char timestamp_buf[30];
  // Format as ISO 8601 local time. If UTC is needed, adjust timeinfo or ensure backend handles local.
  strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%dT%H:%M:%S", &timeinfo); 
  return String(timestamp_buf);
}