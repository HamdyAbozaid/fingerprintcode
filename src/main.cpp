#include <Arduino.h>
#include "config.h"
#include "fingerprint.h"
#include "network.h"
#include "ota.h"
#include "power.h"

void setup() {
    Serial.begin(115200);
    PowerManager::begin();
    FingerprintManager::begin();
    
    if (PowerManager::isRecoveryMode()) {
        PowerManager::enterRecoveryMode();
    }
    
    NetworkManager::autoConnect();
    OTAManager::setup();
}

void loop() {
    OTAManager::handle();
    FingerprintManager::verifyFingerprint();
    PowerManager::manageSleep();
}