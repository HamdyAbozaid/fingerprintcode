#include "OTAModule.h"
#include "globals.h"       // For otaState, lcd, pendingFirmwareURL, expectedHash, etc.
#include "config.h"        // For OTA_PORT, FIRMWARE_VERSION, BACKEND_UPDATE_URL etc.
#include "DisplayModule.h" // For displayOTAProgress, showFingerprintPrompt, recordLCDActivity
#include "SystemModule.h"  // For getDeviceID, isBatterySufficient, getBatteryLevel
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <mbedtls/md.h>
#include <ArduinoOTA.h>
#include <ESP.h> // For ESP.restart()

void setupOTA() {
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setHostname(getDeviceID().c_str());
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setRebootOnSuccess(true);
  ArduinoOTA.setMdnsEnabled(true);

  ArduinoOTA.onStart([]() {
    otaState = OTA_UPDATING;
    otaStartTime = millis();
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "FileSystem";
    Serial.println("OTA Start: " + type);
    lcd.clear();
    lcd.print("OTA: " + type);
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    digitalWrite(SENSOR_PWR_PIN, LOW);
    recordLCDActivity();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    displayOTAProgress(progress, total, "Local Update");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
    otaState = OTA_IDLE;
    digitalWrite(SENSOR_PWR_PIN, HIGH);
    lcd.clear();
    lcd.print("OTA Complete!");
  });

  ArduinoOTA.onError(handleOTAError);
  ArduinoOTA.begin();
  Serial.println("Local OTA service initialized. Hostname: " + getDeviceID());
}

void checkForUpdates() {
  if (!isBatterySufficient()) { Serial.println("OTA Check: Battery low."); return; }
  if (WiFi.status() != WL_CONNECTED) { Serial.println("OTA Check: WiFi not connected."); return; }
  if (otaState != OTA_IDLE) { Serial.println("OTA Check: OTA busy."); return; }

  Serial.println("Checking backend for updates...");
  lcd.clear(); lcd.print("Checking Update"); recordLCDActivity();

  WiFiClientSecure client;
  client.setCACert(ROOT_CA_CERTIFICATE);
  HTTPClient https;
  https.setTimeout(15000);

  if (https.begin(client, BACKEND_UPDATE_URL)) {
    https.addHeader("X-Device-ID", getDeviceID());
    https.addHeader("X-Firmware-Version", FIRMWARE_VERSION);
    https.addHeader("X-Battery-Level", String(getBatteryLevel()));

    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Update check response received.");
      processUpdateResponse(https.getString());
    } else if (httpCode == HTTP_CODE_NO_CONTENT) {
      Serial.println("No update available (HTTP 204 No Content).");
      lcd.clear(); lcd.print("No New Update"); delay(1500); showFingerprintPrompt();
    } else {
      Serial.printf("Update check HTTP Error: %d - %s\n", httpCode, https.errorToString(httpCode).c_str());
      lcd.clear(); lcd.print("Update Chk Err"); lcd.setCursor(0,1); lcd.print("Code:" + String(httpCode)); delay(2000); showFingerprintPrompt();
    }
    https.end();
  } else {
    Serial.println("HTTP begin failed for update URL.");
    lcd.clear(); lcd.print("Update URL Err"); delay(2000); showFingerprintPrompt();
  }
}

void processUpdateResponse(String payload) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: ")); Serial.println(error.f_str());
    lcd.clear(); lcd.print("Update JSON Err"); lcd.setCursor(0,1); lcd.print(error.f_str()); delay(2000); showFingerprintPrompt();
    return;
  }

  if (doc["update_available"] == true) {
    String firmwareUrl = doc["firmware_url"].as<String>();
    String firmwareHash = doc["firmware_hash"].as<String>();
    int firmwareSize = doc["firmware_size"].as<int>();
    String newVersion = doc["version"].as<String>();

    if (firmwareUrl.isEmpty() || firmwareHash.isEmpty() || firmwareSize <= 0 || newVersion.isEmpty()) {
        Serial.println("Incomplete update information received.");
        lcd.clear(); lcd.print("Bad Update Info"); delay(2000); showFingerprintPrompt();
        return;
    }
    if (newVersion == FIRMWARE_VERSION) {
        Serial.println("Update available, but version is same. Ignoring.");
        lcd.clear(); lcd.print("Version is Same"); delay(2000); showFingerprintPrompt();
        return;
    }

    Serial.printf("Update available: v%s, URL: %s, Hash: %s, Size: %d bytes\n",
                  newVersion.c_str(), firmwareUrl.c_str(), firmwareHash.c_str(), firmwareSize);

    if (confirmUpdate(newVersion, firmwareSize)) {
      pendingFirmwareURL = firmwareUrl;
      expectedHash = firmwareHash;
      startDownloadWithVerification();
    } else {
      Serial.println("Firmware update declined or timed out.");
      showFingerprintPrompt();
    }
  } else {
    Serial.println("No firmware update available.");
    lcd.clear(); lcd.print("No New Update"); delay(1500); showFingerprintPrompt();
  }
}

void startDownloadWithVerification() {
  if (pendingFirmwareURL.isEmpty() || expectedHash.isEmpty()) {
    Serial.println("Download cannot start: Firmware URL or Hash is missing.");
    otaState = OTA_IDLE; showFingerprintPrompt(); return;
  }
   if (!isBatterySufficient()) {
    Serial.println("Download cannot start: Battery too low.");
    lcd.clear(); lcd.print("Battery Low!"); lcd.setCursor(0,1); lcd.print("Update Aborted"); delay(2000);
    otaState = OTA_IDLE; showFingerprintPrompt(); return;
  }

  otaState = OTA_DOWNLOADING;
  otaStartTime = millis();
  Serial.printf("Starting firmware download from: %s\n", pendingFirmwareURL.c_str());

  WiFiClientSecure client;
  client.setCACert(ROOT_CA_CERTIFICATE);
  HTTPClient https;
  https.setTimeout(600000); // 10 minutes

  if (https.begin(client, pendingFirmwareURL)) {
    int httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = https.getSize();
      if (contentLength <= 0) {
          Serial.println("Content length error. Aborting update.");
          lcd.clear(); lcd.print("DL Size Error"); delay(2000);
          https.end(); otaState = OTA_IDLE; showFingerprintPrompt(); return;
      }

      if (Update.begin(contentLength, U_FLASH)) {
        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        if(mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0) != 0){
            Serial.println("mbedtls_md_setup failed. Aborting.");
            Update.end(false); https.end(); otaState = OTA_IDLE;
            lcd.clear(); lcd.print("Hash Init Fail"); delay(2000); showFingerprintPrompt(); return;
        }
        mbedtls_md_starts(&ctx);

        WiFiClient *stream = https.getStreamPtr();
        uint8_t buffer[1024];
        size_t totalRead = 0;

        lcd.clear(); lcd.print("Downloading FW");
        digitalWrite(SENSOR_PWR_PIN, LOW); recordLCDActivity();

        while (https.connected() && totalRead < contentLength) {
          size_t availableBytes = stream->available();
          if (availableBytes) {
            size_t readLen = min(availableBytes, sizeof(buffer));
            size_t bytesRead = stream->readBytes(buffer, readLen);
            if (bytesRead > 0) {
              mbedtls_md_update(&ctx, buffer, bytesRead);
              if (Update.write(buffer, bytesRead) != bytesRead) {
                Serial.println("Update.write error!"); Update.end(false); mbedtls_md_free(&ctx); https.end();
                otaState = OTA_IDLE; lcd.clear(); lcd.print("UpdateWriteFail"); delay(2000);
                digitalWrite(SENSOR_PWR_PIN, HIGH); showFingerprintPrompt(); return;
              }
              totalRead += bytesRead;
              displayOTAProgress(totalRead, contentLength, "FW Download");
            } else if (bytesRead < 0) {
                Serial.println("Stream read error!"); Update.end(false); mbedtls_md_free(&ctx); https.end();
                otaState = OTA_IDLE; lcd.clear(); lcd.print("UpdateReadFail"); delay(2000);
                digitalWrite(SENSOR_PWR_PIN, HIGH); showFingerprintPrompt(); return;
            }
          }
          yield();
        }
        digitalWrite(SENSOR_PWR_PIN, HIGH);

        if (totalRead != contentLength && contentLength > 0) {
            Serial.printf("Download incomplete. Read %d, expected %d\n", totalRead, contentLength);
            Update.end(false); mbedtls_md_free(&ctx); https.end();
            otaState = OTA_IDLE; lcd.clear(); lcd.print("DL Incomplete"); delay(2000); showFingerprintPrompt(); return;
        }

        otaState = OTA_VERIFYING;
        unsigned char calculatedHashOutput[32];
        mbedtls_md_finish(&ctx, calculatedHashOutput);
        mbedtls_md_free(&ctx);

        char calculatedHashString[65];
        for (int i = 0; i < 32; i++) { sprintf(calculatedHashString + (i * 2), "%02x", calculatedHashOutput[i]); }
        calculatedHashString[64] = '\0';

        Serial.printf("Expected Hash:   %s\n", expectedHash.c_str());
        Serial.printf("Calculated Hash: %s\n", calculatedHashString);
        lcd.clear(); lcd.print("Verifying Hash..."); recordLCDActivity();

        if (expectedHash.equalsIgnoreCase(calculatedHashString)) {
          Serial.println("Hash OK. Applying update.");
          lcd.clear(); lcd.print("Verify OK"); lcd.setCursor(0,1); lcd.print("Applying Update"); delay(1000); recordLCDActivity();
          otaState = OTA_UPDATING;
          if (Update.end(true)) {
            Serial.println("Update successful! Rebooting...");
            lcd.clear(); lcd.print("Update Success!"); lcd.setCursor(0,1); lcd.print("Rebooting..."); delay(2000);
            ESP.restart();
          } else {
            Serial.printf("Update.end() failed! Err: %u\n", Update.getError());
            lcd.clear(); lcd.print("UpdateApplyFail"); lcd.setCursor(0,1); lcd.print("Err: " + String(Update.getError())); delay(3000);
          }
        } else {
          Serial.println("Hash FAILED! Firmware corrupt."); Update.end(false);
          lcd.clear(); lcd.print("Update HashFail!"); lcd.setCursor(0,1); lcd.print("Aborting."); delay(3000);
        }
      } else {
        Serial.printf("Update.begin failed! Err: %u (Len: %d)\n", Update.getError(), contentLength);
        lcd.clear(); lcd.print("Update InitFail"); lcd.setCursor(0,1); lcd.print("Err: " + String(Update.getError())); delay(3000);
      }
    } else {
      Serial.printf("FW download HTTP Err: %d - %s\n", httpCode, https.errorToString(httpCode).c_str());
      lcd.clear(); lcd.print("Download Fail"); lcd.setCursor(0,1); lcd.print("Code: " + String(httpCode)); delay(2000);
    }
    https.end();
  } else {
    Serial.println("HTTP begin failed for FW download URL.");
    lcd.clear(); lcd.print("DL URL Error"); delay(2000);
  }

  otaState = OTA_IDLE;
  digitalWrite(SENSOR_PWR_PIN, HIGH);
  showFingerprintPrompt();
}

bool confirmUpdate(String newVersion, int firmwareSize) {
  lcd.clear();
  lcd.print("New FW: v" + newVersion.substring(0, min((int)newVersion.length(), (int)(16-9) )));
  lcd.setCursor(0, 1);
  char sizeStr[16];
  sprintf(sizeStr, "Size: %dKB", firmwareSize / 1024);
  lcd.print(String(sizeStr));
  recordLCDActivity();
  delay(2000);

  lcd.clear();
  lcd.print("Update FW?");
  lcd.setCursor(0,1);
  lcd.print("Hold BTN for Yes");
  recordLCDActivity();

  unsigned long confirmStartTime = millis();
  const unsigned long confirmTimeout = 20000;

  while (millis() - confirmStartTime < confirmTimeout) {
    debouncer.update();
    if (debouncer.read() == LOW) {
        if (debouncer.currentDuration() >= 1500) {
            Serial.println("Update confirmed by user.");
            lcd.clear(); lcd.print("Update Confirmed"); delay(1000);
            return true;
        }
    }
    if (debouncer.rose() && debouncer.previousDuration() < 1500 && debouncer.previousDuration() > 50) {
        Serial.println("Update cancelled by user.");
        lcd.clear(); lcd.print("Update Cancelled"); delay(1000);
        return false;
    }
    yield();
  }

  Serial.println("Update confirmation timed out.");
  lcd.clear(); lcd.print("Update Timeout"); delay(1000);
  return false;
}

void handleOTAError(ota_error_t error) {
  otaState = OTA_IDLE;
  digitalWrite(SENSOR_PWR_PIN, HIGH);

  Serial.printf("OTA Error[%u]: ", error);
  lcd.clear();
  lcd.print("OTA Error!");
  lcd.setCursor(0, 1);
  String errorMsg = "Unknown #" + String(error);
  if (error == OTA_AUTH_ERROR) errorMsg = "Auth Failed";
  else if (error == OTA_BEGIN_ERROR) errorMsg = "Begin Failed";
  else if (error == OTA_CONNECT_ERROR) errorMsg = "Connect Failed";
  else if (error == OTA_RECEIVE_ERROR) errorMsg = "Receive Failed";
  else if (error == OTA_END_ERROR) errorMsg = "End Failed";

  Serial.println(errorMsg);
  lcd.print(errorMsg.substring(0, 16));
  delay(3000);
  showFingerprintPrompt();
}

void startSecureRecoveryUpdate(String updateInfo) {
    Serial.printf("Attempting secure recovery update with info: %s\n", updateInfo.c_str());
    lcd.clear(); lcd.print("Recovery Update"); recordLCDActivity();

    if (WiFi.status() != WL_CONNECTED || !isBatterySufficient()) {
        if(WiFi.status() != WL_CONNECTED) {lcd.setCursor(0,1); lcd.print("No WiFi for Rec");}
        else {lcd.setCursor(0,1); lcd.print("Low Batt for Rec");}
        Serial.println("Recovery Update: Pre-checks failed (WiFi/Battery).");
        delay(2000); showFingerprintPrompt(); return;
    }

    int separatorPos = updateInfo.indexOf(';');
    if (separatorPos == -1 || separatorPos == 0 || separatorPos == updateInfo.length() - 1) {
        Serial.println("Invalid recovery update info format.");
        lcd.setCursor(0,1); lcd.print("Bad Rec Info Fmt"); delay(2000);
        showFingerprintPrompt(); return;
    }

    pendingFirmwareURL = updateInfo.substring(0, separatorPos);
    expectedHash = updateInfo.substring(separatorPos + 1);

    if (pendingFirmwareURL.isEmpty() || expectedHash.isEmpty()) {
        Serial.println("Recovery URL or Hash is empty after parsing.");
        lcd.setCursor(0,1); lcd.print("Rec URL/Hash Err"); delay(2000);
        showFingerprintPrompt(); return;
    }

    Serial.printf("Recovery Download: URL=%s, Hash=%s\n", pendingFirmwareURL.c_str(), expectedHash.c_str());
    startDownloadWithVerification();
}
