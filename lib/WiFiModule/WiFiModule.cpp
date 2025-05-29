#include "WiFiModule.h"
#include "globals.h"       // For lcd
#include "config.h"        // For LCD_BACKLIGHT_PIN
#include "DisplayModule.h" // For recordLCDActivity
#include <WiFiManager.h>
#include <WiFi.h>          // For WiFi.status(), WiFi.localIP() etc.
#include <ESP.h>           // For ESP.restart()

bool autoConnectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setTitle("Fingerprint Setup");
  wm.setConnectTimeout(30);
  wm.setDebugOutput(true);

  lcd.clear();
  lcd.print("Connecting WiFi");
  lcd.setCursor(0,1);
  lcd.print("AutoConnect...");
  recordLCDActivity();

  if (wm.autoConnect()) {
    lcd.clear();
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    Serial.print("WiFi Connected! IP: ");
    Serial.println(WiFi.localIP());
    delay(1500);
    return true;
  }
  Serial.println("WiFi AutoConnect Failed. Portal might have started or timed out.");
  return false;
}

void startWiFiManager(bool forceConfig) {
  Serial.println("Starting WiFiManager config portal.");
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  lcd.clear();
  lcd.print("WiFi Config Mode");
  lcd.setCursor(0,1);
  lcd.print("AP: ESP_XXXXXX"); // Generic name, actual depends on WiFiManager
  recordLCDActivity();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setDebugOutput(true);

  bool connected = false;
  if (forceConfig) {
    connected = wm.startConfigPortal("ESP32-Fingerprint-Setup");
  } else {
    connected = wm.autoConnect("ESP32-Fingerprint-Setup");
  }

  Serial.println("WiFiManager finished.");
  if (connected) {
    lcd.clear();
    lcd.print("Config Success!");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("WiFi Configuration failed or timed out.");
    lcd.clear();
    lcd.print("Config Failed");
    lcd.setCursor(0,1);
    lcd.print("No Connection");
    delay(2000);
  }
  Serial.println("Restarting ESP...");
  ESP.restart();
}
