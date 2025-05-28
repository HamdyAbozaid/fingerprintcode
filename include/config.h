#pragma once

// Hardware Pins
constexpr uint8_t BUTTON_PIN = 0;
constexpr uint8_t SENSOR_PWR_PIN = 4;
constexpr uint8_t LCD_BACKLIGHT_PIN = 27;
constexpr uint8_t BATTERY_PIN = 35;

// LCD Pins
constexpr uint8_t LCD_RS = 19;
constexpr uint8_t LCD_EN = 23;
constexpr uint8_t LCD_D4 = 32;
constexpr uint8_t LCD_D5 = 33;
constexpr uint8_t LCD_D6 = 25;
constexpr uint8_t LCD_D7 = 26;

// OTA Configuration
constexpr uint16_t OTA_PORT = 3232;
constexpr const char* OTA_PASSWORD = "f1ng3rpr1nt$ecure";
constexpr const char* FIRMWARE_VERSION = "1.2.0";
constexpr float BATTERY_MIN_VOLTAGE = 3.6;
constexpr unsigned long UPDATE_CHECK_INTERVAL = 21600000; // 6 hours

// Backend URLs
constexpr const char* BACKEND_ATTENDANCE_URL = "https://your_backend_domain:port/api/attendance";
constexpr const char* BACKEND_UPDATE_URL = "https://your_backend_domain:port/api/check-update";

// NTP Configuration
constexpr const char* NTP_SERVER1 = "pool.ntp.org";
constexpr const char* NTP_SERVER2 = "time.nist.gov";
constexpr long GMT_OFFSET_SEC = 2 * 3600; // Egypt GMT+2
constexpr int DAYLIGHT_OFFSET_SEC = 0;

// Timeouts
constexpr unsigned long INACTIVITY_TIMEOUT = 30000; // 30 seconds