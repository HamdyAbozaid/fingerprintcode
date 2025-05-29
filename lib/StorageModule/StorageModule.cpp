#include "StorageModule.h"
#include "globals.h"        // For filesys, isOffline
#include "BackendModule.h"  // For sendToBackend
#include "DisplayModule.h"  // For LCD messages, showFingerprintPrompt, recordLCDActivity
#include <WiFi.h>           // For WiFi.status()

void logAttendanceOffline(int userId, const char* timestamp) {
  Serial.printf("Logging offline to Ext Flash: ID %d, Time %s\n", userId, timestamp);
  File file = filesys.open("/offline_logs.txt", FILE_APPEND);
  if (file) {
    file.printf("%d,%s\n", userId, timestamp);
    file.close();
    Serial.println("Offline log saved to External Flash.");
  } else {
    Serial.println("Failed to open offline_logs.txt on External Flash for writing.");
    lcd.clear(); lcd.print("Ext.Log Error!"); delay(1000);
  }
}

void syncOfflineLogs() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot sync offline logs: WiFi not connected.");
    return;
  }

  Serial.println("Attempting to sync offline logs from External Flash...");
  lcd.clear();
  lcd.print("Syncing ExtLogs");
  recordLCDActivity();

  if (!filesys.exists("/offline_logs.txt")) {
    Serial.println("No offline_logs.txt file found on External Flash.");
    isOffline = false;
    lcd.clear(); lcd.print("No Ext.Logs"); delay(1500);
    showFingerprintPrompt();
    return;
  }
  
  File file = filesys.open("/offline_logs.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open offline_logs.txt on External Flash for reading.");
    lcd.clear(); lcd.print("Ext.LogReadErr"); delay(1500);
    showFingerprintPrompt();
    return;
  }
  if (file.size() == 0) {
    file.close();
    Serial.println("offline_logs.txt on External Flash is empty.");
    isOffline = false;
    filesys.remove("/offline_logs.txt");
    lcd.clear(); lcd.print("No Ext.Logs"); delay(1500);
    showFingerprintPrompt();
    return;
  }

  String logLine;
  bool allSyncedSuccessfully = true;
  String remainingLogs = "";

  while (file.available()) {
    logLine = file.readStringUntil('\n');
    logLine.trim();
    if (logLine.length() == 0) continue;

    int commaIndex = logLine.indexOf(',');
    if (commaIndex == -1) {
      Serial.printf("Invalid log line format: %s\n", logLine.c_str());
      remainingLogs += logLine + "\n";
      allSyncedSuccessfully = false;
      continue;
    }

    int userId = logLine.substring(0, commaIndex).toInt();
    String timestamp = logLine.substring(commaIndex + 1);

    Serial.printf("Sending offline log from Ext Flash: ID %d, Time %s\n", userId, timestamp.c_str());
    if (sendToBackend(userId, timestamp)) {
      Serial.println("Offline log sent successfully.");
    } else {
      Serial.printf("Failed to send offline log: %s\n", logLine.c_str());
      remainingLogs += logLine + "\n";
      allSyncedSuccessfully = false;
    }
  }
  file.close();

  if (allSyncedSuccessfully && remainingLogs.length() == 0) {
    Serial.println("All offline logs from External Flash synced. Deleting file.");
    filesys.remove("/offline_logs.txt");
    isOffline = false;
    lcd.clear(); lcd.print("Ext.Logs Synced!");
  } else {
    Serial.println("Some offline logs from External Flash failed to sync or remain. Rewriting file.");
    filesys.remove("/offline_logs.txt"); 
    File outFile = filesys.open("/offline_logs.txt", FILE_WRITE);
    if (outFile) {
      outFile.print(remainingLogs);
      outFile.close();
      lcd.clear(); lcd.print("Ext.SyncPartial");
    } else {
      Serial.println("ERROR: Could not rewrite offline_logs.txt on External Flash!");
      lcd.clear(); lcd.print("Ext.SyncSaveErr");
    }
  }
  delay(1500);
  showFingerprintPrompt();
}