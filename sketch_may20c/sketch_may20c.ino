#include <LiquidCrystal.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//modifiy 27 / 5
// إعدادات الواي فاي
#define WIFI_SSID "Alla"
#define WIFI_PASSWORD "GREA@G&R6"

// تعريف أطراف LCD
const int rs = 19, en = 23, d4 = 32, d5 = 33, d6 = 25, d7 = 26;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Serial مخصص لحساس البصمة
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger(&mySerial);

// زر الإضافة فقط
const int addButtonPin = 18;

// هيكل لتخزين بيانات البصمة
struct FingerprintData {
  uint16_t id;
  String name;
  time_t timestamp;
};
FingerprintData fingerprints[128];
uint16_t nextID = 0;
int lastDetectedID = -1;
bool inEnrollmentMode = false;

// إعدادات السيرفر
const String serverBaseUrl = "https://192.168.1.12:7069/api/SensorData";
const int serverTimeout = 10000; // 10 ثواني

void setupWiFi();
void displayMainMenu();
void displayMessage(String line1, String line2 = "", int delayTime = 2000);
bool addFingerprint();
void enrollWithRetry();
void sendToServer(uint16_t id, String name);
void showFingerPositionGuide();
void checkSensorStatus();
void smartDelay(unsigned long ms);
int getFingerprintImage(const char* scanType = "Normal scan");
int waitForFingerRemoval();
void handleImageError(int error);
void handleModelError(int error);
int fetchNextIDFromServer();
String fetchNameFromServer(uint16_t id);
void getFingerprintID();
void handleButtonPress();
void enterEnrollmentMode();
void exitEnrollmentMode();
void printDebug(String message);

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  displayMessage("System Initializing", "Please wait...");
  
  pinMode(addButtonPin, INPUT_PULLUP);

  // تهيئة حساس البصمة
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    displayMessage("Fingerprint Sensor", "Initialized");
  } else {
    displayMessage("Sensor Error", "Check connection");
    while (1) delay(1);
  }

  setupWiFi();
  checkSensorStatus();

  nextID = fetchNextIDFromServer();
  printDebug("Next available ID: " + String(nextID));

  displayMainMenu();
}

void loop() {
  handleButtonPress();
  
  if (!inEnrollmentMode) {
    getFingerprintID();
  }
  
  delay(100);
}

void handleButtonPress() {
  static unsigned long lastPressTime = 0;
  
  // زر الإضافة فقط
  if (digitalRead(addButtonPin) == LOW && millis() - lastPressTime > 500) {
    smartDelay(50);
    if (digitalRead(addButtonPin) == LOW) {
      lastPressTime = millis();
      enterEnrollmentMode();
      enrollWithRetry();
      exitEnrollmentMode();
      displayMainMenu();
    }
  }
}

void enterEnrollmentMode() {
  inEnrollmentMode = true;
  displayMessage("Enrollment Mode", "Started");
}

void exitEnrollmentMode() {
  inEnrollmentMode = false;
  displayMessage("Enrollment Mode", "Ended");
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  displayMessage("Connecting to", WIFI_SSID);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(300);
    lcd.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    displayMessage("WiFi Connected", "IP: " + WiFi.localIP().toString());
  } else {
    displayMessage("WiFi Failed", "Using offline mode");
  }
}

void displayMainMenu() {
  lcd.clear();
  lcd.print("Press Button");
  lcd.setCursor(0, 1);
  lcd.print("to Add Fingerprint");
}

void displayMessage(String line1, String line2, int delayTime) {
  lcd.clear();
  lcd.print(line1);
  if (line2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
  if (delayTime > 0) {
    delay(delayTime);
  }
}

void enrollWithRetry() {
  showFingerPositionGuide();
  
  for (int attempt = 1; attempt <= 3; attempt++) {
    displayMessage("Enrollment Attempt", String(attempt) + "/3");
    
    if (addFingerprint()) {
      displayMessage("Enrollment", "Success!");
      return;
    }
    
    if (attempt < 3) {
      displayMessage("Retrying...", "Attempt " + String(attempt+1) + "/3");
      delay(2000);
    }
  }
  
  displayMessage("Enrollment", "Failed after 3 tries", 3000);
}

bool addFingerprint() {
  // المسح الأول
  int p = getFingerprintImage("First scan");
  if (p != FINGERPRINT_OK) return false;

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    handleImageError(p);
    return false;
  }

  displayMessage("Remove finger", "", 2000);
  p = waitForFingerRemoval();
  if (p != FINGERPRINT_NOFINGER) return false;

  // المسح الثاني
  displayMessage("Place same", "finger again", 1000);
  p = getFingerprintImage("Second scan");
  if (p != FINGERPRINT_OK) return false;

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    handleImageError(p);
    return false;
  }

  // إنشاء النموذج
  p = finger.createModel();
  if (p == FINGERPRINT_ENROLLMISMATCH) {
    displayMessage("Error", "Finger mismatch");
    return false;
  } else if (p != FINGERPRINT_OK) {
    handleModelError(p);
    return false;
  }

  // تخزين النموذج
  uint16_t id = nextID++;
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    displayMessage("Storage Error", "Code: " + String(p));
    return false;
  }

  // إدخال الاسم
  String name = "User_" + String(id); // الاسم الافتراضي
  displayMessage("Enter name", "via Serial Monitor", 0);
  
  unsigned long start = millis();
  while (millis() - start < 30000) {
    if (Serial.available() > 0) {
      name = Serial.readStringUntil('\n');
      name.trim();
      if (name.length() > 0) break;
    }
    delay(50);
  }

  // حفظ البيانات وإرسالها للسيرفر
  fingerprints[id].id = id;
  fingerprints[id].name = name;
  fingerprints[id].timestamp = millis();
  
  sendToServer(id, name);
  return true;
}

void sendToServer(uint16_t id, String name) {
  if (WiFi.status() != WL_CONNECTED) {
    displayMessage("Offline Mode", "Data saved locally");
    return;
  }

  HTTPClient http;
  String url = serverBaseUrl;
  String payload = "{\"id\":" + String(id) + ",\"name\":\"" + name + "\"}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(serverTimeout);

  printDebug("POST URL: " + url);
  printDebug("Payload: " + payload);

  int httpCode = http.POST(payload);
  String response = http.getString();

  printDebug("HTTP Code: " + String(httpCode));
  printDebug("Response: " + response);

  if (httpCode > 0) {
    displayMessage("Data sent to", "server successfully");
  } else {
    displayMessage("Failed to send", "to server");
  }

  http.end();
}

int getFingerprintImage(const char* scanType) {
  int p = -1;
  unsigned long start = millis();
  
  while (p != FINGERPRINT_OK && millis() - start < 10000) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      printDebug(String(scanType) + " successful");
      return p;
    } else if (p != FINGERPRINT_NOFINGER) {
      handleImageError(p);
      return p;
    }
    delay(100);
  }
  
  return FINGERPRINT_IMAGEFAIL;
}

int waitForFingerRemoval() {
  int p = 0;
  unsigned long start = millis();
  
  while (p != FINGERPRINT_NOFINGER && millis() - start < 5000) {
    p = finger.getImage();
    delay(100);
  }
  
  return p;
}

void handleImageError(int error) {
  String errorMsg;
  switch (error) {
    case FINGERPRINT_IMAGEFAIL: errorMsg = "Poor image quality"; break;
    case FINGERPRINT_PACKETRECIEVEERR: errorMsg = "Comm error"; break;
    case FINGERPRINT_NOFINGER: errorMsg = "No finger detected"; break;
    default: errorMsg = "Error code: " + String(error);
  }
  displayMessage("Scan Error", errorMsg);
  printDebug("Image Error: " + errorMsg);
}

void handleModelError(int error) {
  String errorMsg;
  switch (error) {
    case FINGERPRINT_ENROLLMISMATCH: errorMsg = "Finger mismatch"; break;
    case FINGERPRINT_BADLOCATION: errorMsg = "Bad location"; break;
    case FINGERPRINT_FLASHERR: errorMsg = "Flash error"; break;
    default: errorMsg = "Error code: " + String(error);
  }
  displayMessage("Model Error", errorMsg);
  printDebug("Model Error: " + errorMsg);
}

void showFingerPositionGuide() {
  displayMessage("Place finger", "flat & centered", 2000);
}

void checkSensorStatus() {
  displayMessage("Checking", "sensor status...", 1000);
  
  int p = finger.getImage();
  delay(500);
  
  if (p == FINGERPRINT_OK) {
    displayMessage("Sensor Status", "Working fine");
  } else {
    displayMessage("Sensor Status", "Problem detected");
    handleImageError(p);
  }
}

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    delay(50);
  }
}

void getFingerprintID() {
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) return;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    handleImageError(p);
    return;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    uint16_t id = finger.fingerID;
    if (id == lastDetectedID) return; // تجنب التكرار
    
    lastDetectedID = id;
    String name = fetchNameFromServer(id);
    
    displayMessage("ID: " + String(id), "Name: " + name, 3000);
    printDebug("Matched ID: " + String(id) + ", Name: " + name);
  } else if (p == FINGERPRINT_NOTFOUND) {
    displayMessage("No Match", "Finger not found", 1000);
  } else {
    handleImageError(p);
  }
}

int fetchNextIDFromServer() {
  if (WiFi.status() != WL_CONNECTED) {
    printDebug("Offline - Starting ID from 0");
    return 0;
  }

  HTTPClient http;
  String url = serverBaseUrl + "/last-id";
  
  http.begin(url);
  http.setTimeout(serverTimeout);

  int httpCode = http.GET();
  int lastID = 0;

  if (httpCode == 200) {
    lastID = http.getString().toInt();
    printDebug("Last ID from server: " + String(lastID));
  } else {
    printDebug("Failed to fetch last ID. HTTP code: " + String(httpCode));
  }

  http.end();
  return lastID + 1;
}

String fetchNameFromServer(uint16_t id) {
  // التحقق من التخزين المحلي أولاً
  if (fingerprints[id].id == id && fingerprints[id].name != "") {
    return fingerprints[id].name;
  }

  if (WiFi.status() != WL_CONNECTED) {
    return "Offline User";
  }

  HTTPClient http;
  String url = serverBaseUrl + "/" + String(id);
  
  http.begin(url);
  http.setTimeout(serverTimeout);

  int httpCode = http.GET();
  String name = "Unknown";

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    name = doc["name"].as<String>();
    
    // تحديث التخزين المحلي
    fingerprints[id].id = id;
    fingerprints[id].name = name;
    fingerprints[id].timestamp = millis();
  } else {
    printDebug("Failed to fetch name. HTTP code: " + String(httpCode));
  }

  http.end();
  return name;
}

void printDebug(String message) {
  Serial.println("[DEBUG] " + message);
}