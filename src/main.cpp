// src/main.cpp

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Bounce2.h>
#include <LiquidCrystal.h>
#include <Adafruit_Fingerprint.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <ArduinoOTA.h> // For ArduinoOTA.handle()

// Include global configuration header
#include "config.h"

// Include custom modules
#include "Utils.h"
#include "PowerManagement.h"
#include "OTAManager.h"
#include "LCDManager.h"
#include "OfflineLogger.h"

// Global Instances (declared extern in headers)
// Initialize with values from config.h
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
HardwareSerial mySerial(2); // Use UART2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
Bounce debouncer = Bounce();

// Global Variables (these are state variables, not configuration)
RTC_DATA_ATTR int bootCount = 0;
bool isOffline = false;
unsigned long lastActivity = 0;
unsigned long lastSyncAttempt = 0;

// Forward declarations for functions defined in main.cpp if they call each other
void initSystem();
void verifyFingerprint();

void setup() {
    Serial.begin(115200);

    // Initialize hardware pins using definitions from config.h
    pinMode(SENSOR_PWR_PIN, OUTPUT);
    digitalWrite(SENSOR_PWR_PIN, HIGH); // Ensure sensor is powered
    pinMode(LCD_BACKLIGHT_PIN, OUTPUT);
    digitalWrite(LCD_BACKLIGHT_PIN, HIGH); // Turn on LCD backlight initially
    pinMode(BATTERY_PIN, INPUT); // Configure battery monitoring pin

    // Initialize LCD (uses pins from config.h)
    LCDManager::begin(16, 2);
    LCDManager::print("System Booting");

    // Button setup with debouncing
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Button connected to GND, so use PULLUP
    debouncer.attach(BUTTON_PIN);
    debouncer.interval(25); // Debounce delay

    // Check for recovery mode (button held down during boot)
    if (digitalRead(BUTTON_PIN) == LOW) {
        OTAManager::enterRecoveryMode(); // This is a blocking function
    }

    // Initialize SPIFFS (flash file system)
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed! Please reboot or check flash.");
        LCDManager::clear();
        LCDManager::print("Storage Error!");
        delay(2000);
        while(1);
    }
    Serial.println("SPIFFS mounted successfully.");

    // Initialize fingerprint sensor
    mySerial.begin(FINGER_BAUD_RATE, SERIAL_8N1, FINGER_RX_PIN, FINGER_TX_PIN);
    if (!finger.begin(FINGER_BAUD_RATE)) {
        Serial.println("Did not find fingerprint sensor :(");
        LCDManager::clear();
        LCDManager::print("Sensor Error!");
        while(1);
    }
    Serial.println("Found fingerprint sensor!");
    finger.setSecurityLevel(FINGERPRINT_SECURITY_LOW); // Adjust as needed

    // Configure wake sources for deep sleep
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW); // Assumes BUTTON_PIN is GPIO_NUM_0

    // Determine the cause of wakeup
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        bootCount++;
        Serial.printf("Boot count: %d\n", bootCount);
        initSystem();
    } else {
        Serial.printf("Wakeup cause: %d. Entering deep sleep.\n", wakeup_reason);
        PowerManagement::enterDeepSleep();
    }
}

void loop() {
    ArduinoOTA.handle();
    debouncer.update();
    PowerManagement::manageLCDPower(LCD_BACKLIGHT_PIN);

    // If an OTA download is in progress, pause other operations
    if (OTAManager::getOtaState() == OTA_DOWNLOADING) {
        return;
    }

    // Check for button press: 3-second hold to force WiFi Manager config portal
    if (debouncer.fell() && debouncer.currentDuration() >= 3000) {
        Serial.println("Button held for 3s. Forcing WiFiManager configuration.");
        Utils::startWiFiManager(true);
        PowerManagement::recordLCDActivity(LCD_BACKLIGHT_PIN);
    } else if (debouncer.fell()) {
        PowerManagement::recordLCDActivity(LCD_BACKLIGHT_PIN);
    }

    // Normal system operation (only if system initialized after boot/wakeup)
    if (bootCount > 0) {
        verifyFingerprint();

        // Check for backend updates periodically
        static unsigned long lastUpdateCheck = 0;
        if (millis() - lastUpdateCheck > UPDATE_CHECK_INTERVAL) { // Use constant from config.h
            Serial.println("Checking for firmware updates from backend...");
            OTAManager::checkForUpdates();
            lastUpdateCheck = millis();
        }

        // Attempt to sync offline logs if in offline mode
        if (isOffline && (millis() - lastSyncAttempt > 30000 || lastSyncAttempt == 0)) {
            Serial.println("Attempting to sync offline logs.");
            OfflineLogger::syncOfflineLogs(isOffline);
            lastSyncAttempt = millis();
        }

        // Enter deep sleep after a period of inactivity if not in offline mode
        if (millis() > INACTIVITY_TIMEOUT && !isOffline) { // Use constant from config.h
            Serial.println("Inactivity timeout reached. Entering deep sleep.");
            PowerManagement::enterDeepSleep();
        }
    }

    delay(10);
}

// Initializes core system components after boot/wakeup
void initSystem() {
    Serial.println("Initializing System...");
    LCDManager::clear();
    LCDManager::print("Initializing...");
    PowerManagement::recordLCDActivity(LCD_BACKLIGHT_PIN);

    // Attempt to connect to saved WiFi credentials
    if (!Utils::autoConnectWiFi()) {
        Serial.println("AutoConnect failed. Starting WiFiManager for configuration.");
        Utils::startWiFiManager(false);
    } else {
        // If WiFi connected, set up time synchronization and local OTA
        Utils::setupNTP();
        OTAManager::setupOTA();
    }
}

// Manages the fingerprint scanning and attendance logging process
void verifyFingerprint() {
    LCDManager::showFingerprintPrompt();
    PowerManagement::recordLCDActivity(LCD_BACKLIGHT_PIN);

    int p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        if (p == FINGERPRINT_NOFINGER) return;
        Serial.printf("getImage error: %d\n", p);
        LCDManager::clear();
        LCDManager::print("Image Error");
        delay(1000);
        return;
    }

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        Serial.printf("image2Tz error: %d\n", p);
        LCDManager::clear();
        LCDManager::print("Convert Error");
        delay(1000);
        return;
    }

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
        int userId = finger.fingerID;
        Serial.printf("Found ID #%d with confidence %d\n", userId, finger.confidence);

        String timestamp = Utils::getTimestamp();
        Serial.printf("Attendance attempt for ID %d at %s\n", userId, timestamp.c_str());

        if (WiFi.status() == WL_CONNECTED) {
            if (Utils::sendToBackend(userId, timestamp)) {
                LCDManager::showAttendanceResult(true, userId);
                isOffline = false;
                return;
            } else {
                Serial.println("Failed to send to backend, falling back to offline logging.");
            }
        } else {
            Serial.println("WiFi not connected, logging offline.");
        }

        isOffline = true;
        OfflineLogger::logAttendanceOffline(userId, timestamp.c_str());
        LCDManager::showOfflineWarning();

    } else if (p == FINGERPRINT_NOFINGER) {
        LCDManager::showAttendanceResult(false, 0);
    } else {
        Serial.printf("Fingerprint search error: %d\n", p);
        LCDManager::showAttendanceResult(false, 0);
    }
}