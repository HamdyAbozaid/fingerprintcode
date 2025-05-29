#include "PowerModule.h"
#include "globals.h"    // For lcd, bootCount, lastActivity
#include "config.h"     // For SENSOR_PWR_PIN, LCD_BACKLIGHT_PIN, LCD_BACKLIGHT_TIMEOUT
#include <WiFi.h>       // For WiFi.mode()
#include <esp_bt.h>     // For btStop() if using classic BT, or esp_bluedroid_disable() etc. for BLE
#include <esp_sleep.h>
#include <driver/rtc_io.h> // For rtc_gpio_hold_en

// For ESP32, btStop() is usually for Classic Bluetooth.
// For BLE, you might need:
// #include "esp_bt.h"
// #include "esp_bluedroid_api.h"
// esp_bluedroid_disable();
// esp_bluedroid_deinit();
// esp_bt_controller_disable();
// esp_bt_controller_deinit();
// For simplicity, using btStop() which might cover some cases or be a placeholder.

void enterDeepSleep() {
  Serial.println("Entering Deep Sleep...");
  lcd.clear();
  lcd.print("Entering Sleep");
  delay(500);

  powerDownPeripherals();

  Serial.printf("Going to sleep now (Boot count: %d)\n", bootCount);
  Serial.println("Wake up by EXT0 (GPIO_NUM_0 LOW).");
  Serial.flush();
  esp_deep_sleep_start();
}

void powerDownPeripherals() {
  Serial.println("Powering down peripherals...");
  digitalWrite(SENSOR_PWR_PIN, LOW);
  rtc_gpio_hold_en((gpio_num_t)SENSOR_PWR_PIN); // Ensure SENSOR_PWR_PIN is held low

  WiFi.mode(WIFI_OFF); // Turn off WiFi radio
  btStop();            // Turn off Bluetooth radio (basic)

  lcd.noDisplay();     // Turn off LCD display content
  digitalWrite(LCD_BACKLIGHT_PIN, LOW); // Turn off LCD backlight
}

void manageLCDPower() {
  if (digitalRead(LCD_BACKLIGHT_PIN) == HIGH && (millis() - lastActivity > LCD_BACKLIGHT_TIMEOUT)) {
    Serial.println("LCD backlight timeout. Turning off backlight.");
    digitalWrite(LCD_BACKLIGHT_PIN, LOW);
  }
}
