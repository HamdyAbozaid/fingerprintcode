// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h> // Include Arduino.h for types like `int`, `unsigned long`, etc.

// --- Hardware Pins ---
#define BUTTON_PIN          0       // GPIO pin for the user button
#define SENSOR_PWR_PIN      4       // GPIO pin to control power to the fingerprint sensor
#define LCD_BACKLIGHT_PIN   27      // GPIO pin for controlling LCD backlight
#define BATTERY_PIN         35      // Analog pin for battery voltage monitoring


// --- LCD Pins ---
#define LCD_RS = 19
#define LCD_EN = 23
#define LCD_D4 = 32
#define LCD_D5 = 33
#define LCD_D6 = 25
#define LCD_D7 = 26

// --- Fingerprint Sensor UART ---
// Note: These are ESP32 pins, not the sensor module pins
#define FINGER_RX_PIN       16      // ESP32 RX2 connected to Finger TX
#define FINGER_TX_PIN       17      // ESP32 TX2 connected to Finger RX
#define FINGER_BAUD_RATE    57600   // Baud rate for fingerprint sensor communication

// --- OTA Configuration ---
#define OTA_PASSWORD        "f1ng3rpr1nt$ecure" // Password for local OTA updates
#define OTA_PORT            3232                // Port for local OTA updates
#define FIRMWARE_VERSION    "1.0.0"             // Current firmware version
#define UPDATE_CHECK_INTERVAL 21600000UL        // Interval to check for backend updates (6 hours in ms)

// --- Backend URLs ---
const char* BACKEND_ATTENDANCE_URL = "https://your_backend_domain:port/api/attendance";
const char* BACKEND_UPDATE_URL     = "https://your_backend_domain:port/api/check-update";

// --- Root CA Certificate for HTTPS ---
// IMPORTANT: Replace with your actual Root CA Certificate for your backend.
// This is crucial for secure HTTPS communication.
const char* ROOT_CA_CERTIFICATE = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA3OgAwIBAgIRAIIQz7svqnVUchfG6iPWCmYwDQYJKoZIhvcNAQELBQAw
... (Your actual Root CA Certificate content goes here) ...
-----END CERTIFICATE-----
)EOF";

// --- NTP Configuration ---
const long GMT_OFFSET_SEC    = 2 * 3600; // GMT +2 hours for EEST (Eastern European Summer Time)
const int DAYLIGHT_OFFSET_SEC = 0;      // No additional daylight saving offset configured here
const char* NTP_SERVER1      = "pool.ntp.org";
const char* NTP_SERVER2      = "time.nist.gov";

// --- Timeouts ---
const unsigned long INACTIVITY_TIMEOUT = 300000UL; // 5 minutes (300 seconds) for deep sleep trigger

// --- Battery Monitoring ---
const float BATTERY_MIN_VOLTAGE = 3.3F; // Minimum required battery voltage for operation/updates

#endif // CONFIG_H