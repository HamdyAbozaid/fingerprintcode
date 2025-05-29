#include "BackendModule.h"
#include "config.h"        // For BACKEND_ATTENDANCE_URL, ROOT_CA_CERTIFICATE, FIRMWARE_VERSION
#include "SystemModule.h"  // For getDeviceID, getBatteryLevel
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool sendToBackend(int userId, String timestamp) {
  Serial.printf("Sending to backend: User ID %d, Timestamp %s\n", userId, timestamp.c_str());
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("sendToBackend: WiFi not connected.");
    return false;
  }

  WiFiClientSecure client;
  client.setCACert(ROOT_CA_CERTIFICATE);
  // client.setInsecure(); // For testing ONLY
  HTTPClient http;
  http.setTimeout(10000); // 10s timeout

  if (!http.begin(client, BACKEND_ATTENDANCE_URL)) {
    Serial.println("HTTP begin failed for attendance URL.");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-ID", getDeviceID());
  http.addHeader("X-Firmware-Version", FIRMWARE_VERSION);

  StaticJsonDocument<192> doc; // Ensure size is adequate
  doc["userId"] = userId;
  doc["timestamp"] = timestamp;
  doc["batteryLevel"] = getBatteryLevel();
  doc["deviceId"] = getDeviceID();

  String requestBody;
  serializeJson(doc, requestBody);
  Serial.print("Sending JSON: "); Serial.println(requestBody);

  int httpResponseCode = http.POST(requestBody);
  bool success = false;
  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    String payload = http.getString();
    Serial.println("HTTP Response payload: " + payload);
    if (httpResponseCode == HTTP_CODE_OK || httpResponseCode == HTTP_CODE_CREATED) {
      success = true;
    }
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  return success;
}