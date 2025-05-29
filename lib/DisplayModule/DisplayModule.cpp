#include "DisplayModule.h"
#include "globals.h" // For lcd object and lastActivity
#include "config.h"  // For LCD_BACKLIGHT_PIN

void recordLCDActivity() {
  if (digitalRead(LCD_BACKLIGHT_PIN) == LOW) {
    digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
    lcd.display();
  }
  lastActivity = millis();
}

void showFingerprintPrompt() {
  lcd.clear();
  lcd.print("Scan Fingerprint");
  lcd.setCursor(0, 1);
  lcd.print("Waiting...");
  recordLCDActivity();
}

void showAttendanceResult(bool success, int userId) {
  lcd.clear();
  if (success) {
    lcd.print("Attendance OK");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(userId);
  } else {
    lcd.print("Not Recognized");
    lcd.setCursor(0, 1);
    lcd.print("Try Again");
  }
  recordLCDActivity();
  delay(2000);
  showFingerprintPrompt(); // Return to prompt
}

void showOfflineWarning() {
  lcd.clear();
  lcd.print("OFFLINE MODE");
  lcd.setCursor(0, 1);
  lcd.print("Data Stored Loc.");
  recordLCDActivity();
  delay(1500);
}

void displayOTAProgress(size_t progress, size_t total, const char* label) {
  static int lastPercentage = -1;
  int percentage = (total > 0) ? (progress * 100 / total) : 0;

  if (percentage != lastPercentage || lastPercentage == -1) {
    lastPercentage = percentage;
    Serial.printf("OTA Progress (%s): %u / %u (%d%%)\n", label, progress, total, percentage);
    lcd.clear();
    char shortLabel[10];
    strncpy(shortLabel, label, 9);
    shortLabel[9] = '\0';
    lcd.print(shortLabel);

    lcd.setCursor(0, 1);
    char progressStr[16];
    sprintf(progressStr, "Progress: %3d%%", percentage);
    lcd.print(progressStr);
    recordLCDActivity();
  }
}
