#ifndef CONFIG_H
#define CONFIG_H

// Hardware Pins
#define BUTTON_PIN 0
#define SENSOR_PWR_PIN 4
#define LCD_BACKLIGHT_PIN 27
#define BATTERY_PIN 35 // ADC1_CH7

// LCD Pins (parallel interface)
#define RS_PIN = 19
#define EN_PIN = 23
#define D4_PIN = 32
#define D5_PIN = 33
#define D6_PIN = 25
#define D7_PIN = 26

// External SPI Flash Pins (using HSPI by default for these defines)
#define FLASH_CS_PIN 15
#define FLASH_SCK_PIN 14  // HSPI SCLK
#define FLASH_MISO_PIN 12 // HSPI MISO
#define FLASH_MOSI_PIN 13 // HSPI MOSI

// OTA Configuration
#define OTA_PASSWORD "f1ng3rpr1nt$ecure"
#define OTA_PORT 3232
#define FIRMWARE_VERSION "1.2.1"
#define PUBLIC_KEY "-----BEGIN PUBLIC KEY-----\nYOUR_PUBLIC_KEY_HERE\n-----END PUBLIC KEY-----"
#define BATTERY_MIN_VOLTAGE 3.6
#define UPDATE_CHECK_INTERVAL 21600000 // 6 hours (6 * 60 * 60 * 1000 ms)

// Power Management
#define INACTIVITY_TIMEOUT 30000       // 30 seconds for deep sleep
#define LCD_BACKLIGHT_TIMEOUT 15000    // 15 seconds for LCD backlight off

// Backend Configuration
#define BACKEND_ATTENDANCE_URL "https://your_backend_domain:port/api/attendance" // REPLACE
#define BACKEND_UPDATE_URL "https://your_backend_domain:port/api/check-update"     // REPLACE

// NTP Configuration
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC (2 * 3600)      // Egypt is GMT+2
#define DAYLIGHT_OFFSET_SEC 0          // No daylight saving currently in Egypt

// Root CA Certificate for HTTPS
// IMPORTANT: Replace this with the actual Root CA Certificate for your backend server.
const char* ROOT_CA_CERTIFICATE = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA3OgAwIBAgIRAIIQz7svqnVUchfG6iPWCmYwDQYJKoZIhvcNAQELBQAw
... (Your actual Root CA certificate content here) ...
-----END CERTIFICATE-----
)EOF"; // REPLACE WITH YOUR ACTUAL ROOT CA CERT

#endif // CONFIG_H