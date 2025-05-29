#include "FingerprintModule.h"
#include "globals.h"         // For finger, otaState, isOffline
#include "DisplayModule.h"   // For showFingerprintPrompt, showAttendanceResult, showOfflineWarning
#include "TimeModule.h"      // For getTimestamp
#include "BackendModule.h"   // For sendToBackend
#include "StorageModule.h"   // For logAttendanceOffline

void verifyFingerprint() {
  if (digitalRead(LCD_BACKLIGHT_PIN) == LOW && otaState == OTA_IDLE) {
      showFingerprintPrompt();
  }

  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) return;
    Serial.printf("getImage error: %d\n", p);
    return;
  }

  recordLCDActivity();

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.printf("image2Tz error: %d\n", p);
    lcd.clear(); lcd.print("Convert Error"); delay(1000);
    showFingerprintPrompt();
    return;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    int userId = finger.fingerID;
    Serial.printf("Found ID #%d with confidence %d\n", userId, finger.confidence);
    String timestamp = getTimestamp();
    Serial.printf("Attendance attempt for ID %d at %s\n", userId, timestamp.c_str());

    bool sentOnline = false;
    if (WiFi.status() == WL_CONNECTED) {
      if (sendToBackend(userId, timestamp)) {
        showAttendanceResult(true, userId);
        isOffline = false;
        sentOnline = true;
        return; 
      } else {
        Serial.println("Failed to send to backend, falling back to offline logging.");
      }
    } else {
      Serial.println("WiFi not connected, logging offline.");
    }

    if (!sentOnline) {
      isOffline = true;
      logAttendanceOffline(userId, timestamp.c_str());
      showOfflineWarning();
      showAttendanceResult(true, userId);
    }

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error with fingerprint sensor");
    lcd.clear(); lcd.print("Sensor Comm Err"); delay(1000);
    showFingerprintPrompt();
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match for the fingerprint.");
    showAttendanceResult(false, 0);
  } else {
    Serial.printf("Unknown fingerprint search error: %d\n", p);
    lcd.clear(); lcd.print("FP Unknown Err"); delay(1000);
    showFingerprintPrompt();
  }
}
