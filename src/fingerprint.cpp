#include "fingerprint.h"
#include <HardwareSerial.h>
#include "network.h"
#include "power.h"

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void FingerprintManager::begin() {
    mySerial.begin(57600, SERIAL_8N1, 16, 17);
    if (!finger.begin(57600)) {
        Serial.println("Fingerprint sensor not found!");
        while(1);
    }
    finger.setSecurityLevel(FINGERPRINT_SECURITY_LOW);
}

// Other fingerprint functions...