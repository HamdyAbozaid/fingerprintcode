#include <Arduino.h>
#include "config.h"    // Project-specific configurations
#include "globals.h"   // Global variables and objects (declarations)

// Include Module Headers
#include "DisplayModule.h"
#include "FingerprintModule.h"
#include "StorageModule.h"
#include "TimeModule.h"
#include "WiFiModule.h"
#include "BackendModule.h"
#include "OTAModule.h"
#include "PowerModule.h"
#include "SystemModule.h"

// Include necessary libraries (some might be covered by globals.h or module headers)
#include <WiFi.h> // For WiFi.status in loop
#include <esp_sleep.h>
#include <driver/rtc_io.h> // For gpio_num_t
#include <ArduinoOTA.h> // For ArduinoOTA.handle()

// --- Global Object Definitions (as declared 'extern' in globals.h) ---
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
HardwareSerial mySerial(2); // UART2 for fingerprint sensor (RX:16, TX:17 default)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

SPIClass hspi(HSPI); // HSPI bus
Adafruit_FlashTransport_SPI flashTransport(FLASH_CS_PIN, &hspi);
Adafruit_SPIFlash flash(&flashTransport);
Adafruit_LittleFS filesys(&flash);

Bounce debouncer = Bounce();

// --- Global Variable Definitions (as declared 'extern' in globals.h) ---
RTC_DATA_ATTR int bootCount = 0;
bool isOffline = false;
unsigned long lastActivity = 0;
unsigned long lastSyncAttempt = 0;

OTAState otaState = OTA_IDLE;
unsigned long otaStartTime = 0;
String pendingFirmwareURL = "";
String expectedHash = "";


void setup() {
  Serial.begin(115200);
  unsigned long serialStartTime = millis();
  while (!Serial && (millis() - serialStartTime < 2000)) { // Wait for serial for debugging
    delay(10);
  }

  // Initialize hardware pins
  pinMode(SENSOR_PWR_PIN, OUTPUT);
  digitalWrite(SENSOR_PWR_PIN, HIGH);
  pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
  pinMode(BATTERY_PIN, INPUT);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("System Booting");
  recordLCDActivity(); // From DisplayModule

  // Initialize Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(25);

  // Check for recovery mode
  delay(50); // Small delay for button signal to stabilize after power-on
  if (digitalRead(BUTTON_PIN) == LOW) {
    enterRecoveryMode(); // From SystemModule
  }

  // Initialize HSPI for External Flash
  // SCK, MISO, MOSI. CS is handled by flashTransport.
  hspi.begin(FLASH_SCK_PIN, FLASH_MISO_PIN, FLASH_MOSI_PIN); 

  // Initialize External Flash Chip
  Serial.println("Initializing External Flash Chip...");
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize External Flash chip!");
    lcd.clear(); lcd.print("Ext.Flash Err"); delay(2000); while(1) yield(); // Halt
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);
  uint32_t flashSize = flash.size();
  Serial.print("Flash size: "); Serial.print(flashSize / (1024 * 1024)); Serial.println(" MB");

  // Initialize LittleFS on External Flash
  Serial.println("Initializing LittleFS on External Flash...");
  if (!filesys.begin()) {
    Serial.println("Failed to mount LittleFS. Formatting...");
    lcd.clear(); lcd.print("Formatting ExtFS"); recordLCDActivity(); delay(1000);
    if (!filesys.format()) {
      Serial.println("Error, failed to format LittleFS on external flash!");
      lcd.clear(); lcd.print("Ext.Format Err"); delay(2000); while(1) yield(); // Halt
    }
    // Try mounting again after formatting
    if (!filesys.begin()) {
        Serial.println("Failed to mount LittleFS even after formatting!");
        lcd.clear(); lcd.print("Ext.Mount Err"); delay(2000); while(1) yield(); // Halt
    }
    Serial.println("LittleFS formatted and mounted successfully.");
  } else {
    Serial.println("LittleFS on External Flash mounted successfully.");
  }

  // Initialize Fingerprint Sensor
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // Fingerprint sensor UART
  if (!finger.begin(57600)) {
    Serial.println("Did not find fingerprint sensor :(");
    lcd.clear(); lcd.print("Sensor Error!"); while(1) yield(); // Halt
  }
  Serial.println("Found fingerprint sensor!");
  finger.setSecurityLevel(FINGERPRINT_SECURITY_LOW); // Adjust as needed

  // Configure deep sleep wake source
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW); // Wake on button press

  // Determine wakeup cause and proceed
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    bootCount++;
    Serial.printf("Boot count: %d\n", bootCount);
    initSystem(); // From SystemModule: Initializes WiFi, NTP, OTA
  } else {
    Serial.printf("Unexpected wakeup cause: %d. Entering deep sleep.\n", wakeup_reason);
    enterDeepSleep(); // From PowerModule
  }
}

void loop() {
  ArduinoOTA.handle(); // Service local OTA updates (from ArduinoOTA library)
  debouncer.update();  // Update button state
  manageLCDPower();    // From PowerModule: Control LCD backlight

  // If an OTA process (downloading, verifying, updating) is active, yield to it
  if (otaState == OTA_DOWNLOADING || otaState == OTA_VERIFYING || otaState == OTA_UPDATING) {
    delay(10); // Small yield to allow OTA process to run
    return;
  }

  // Check for button press: 3-second hold to force WiFi Manager
  if (debouncer.read() == LOW && debouncer.currentDuration() >= 3000) {
     if (debouncer.fell()) { // Trigger once on the transition to held state
        Serial.println("Button held for 3s. Forcing WiFiManager configuration.");
        startWiFiManager(true); // From WiFiModule (will restart ESP)
     }
  } else if (debouncer.fell()) { // Short button press
    Serial.println("Button pressed (short).");
    recordLCDActivity(); // From DisplayModule: Wake up LCD
  }

  // Normal system operation (only if system was initialized)
  if (bootCount > 0) {
    verifyFingerprint(); // From FingerprintModule: Scan for fingerprints

    // Check for backend updates periodically
    static unsigned long lastUpdateCheck = 0;
    // Also check immediately on first loop after init if lastUpdateCheck is 0
    if (millis() - lastUpdateCheck > UPDATE_CHECK_INTERVAL || lastUpdateCheck == 0) {
      if (WiFi.status() == WL_CONNECTED && otaState == OTA_IDLE) { // Only if connected and not in OTA
          Serial.println("Checking for firmware updates from backend...");
          checkForUpdates(); // From OTAModule
      }
      lastUpdateCheck = millis(); // Update check time
    }

    // Attempt to sync offline logs if in offline mode
    if (isOffline && (millis() - lastSyncAttempt > 30000 || lastSyncAttempt == 0)) {
      if (WiFi.status() == WL_CONNECTED) { // Only if connected
        Serial.println("Attempting to sync offline logs.");
        syncOfflineLogs(); // From StorageModule
      }
      lastSyncAttempt = millis(); // Update sync attempt time
    }

    // Enter deep sleep after inactivity if not offline and no OTA in progress
    if (millis() - lastActivity > INACTIVITY_TIMEOUT && !isOffline && otaState == OTA_IDLE) {
      Serial.println("Inactivity timeout reached. Entering deep sleep.");
      enterDeepSleep(); // From PowerModule
    }
  }
  delay(10); // Small delay to yield CPU to other tasks
}