#include <ArduinoOTA.h>
#include "config.h"
#include "PinConfiguration.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

bool isDebug = IS_DEBUG;
#if (IS_DEBUG == true)

#include "Dump.h"

#endif

//Blynk
char *blynkAuthKey = BLYNK_AUTH_KEY;
char *blynkHost = BLYNK_HOST;
int blynkPort = BLYNK_PORT;

//Wifi
char *wifiSsid = WIFI_SSID;
char *wifiPass = WIFI_PASS;
char *wifiHostname = WIFI_HOSTNAME;

//OTA
const boolean isOta = IS_OTA;
const char *otaHost = OTA_HOST;
const char *otaPass = OTA_PASS;

/* BLYNK VIRTUAL PINS */
int timerShortPin = BLYNK_TIMER_SHORT_VIRTUAL_PIN;
int timerShortButtonPin = V2;
int timerLongPin = BLYNK_TIMER_LONG_VIRTUAL_PIN;
int timerLongButtonPin = BLYNK_TIMER_LONG_VIRTUAL_BUTTON_PIN;
/* ///////////////// */

int timerShortValue = TIMER_SHORT_DEFAULT * 1000;
int timerLongValue = TIMER_LONG_DEFAULT * 1000;

int isTimerShortButtonPressed = 0;
int isTimerLongButtonPressed = 0;

unsigned long machineStartTime = 0;
unsigned long relayTriggeredTime = 0;
bool isRelayTriggered = false;

//Wifi Vars
bool isInWifiConnectingMode = false;
int wifiConnectingCount = 0;
unsigned long wifiStartedConnectingTime = 0;

bool isSetupComplete = false;

BLYNK_CONNECTED() {
    Blynk.syncAll();
}

BLYNK_WRITE(timerShortPin){
        timerShortValue = param.asInt() * 1000;
        dump("read timerShortPin: ", false);
        dump(timerShortValue);
}

BLYNK_WRITE(timerShortButtonPin){
        isTimerShortButtonPressed = param.asInt();
        dump("read timerShortButtonPin: ", false);
        dump(isTimerShortButtonPressed);
}

BLYNK_WRITE(timerLongPin){
        timerLongValue = param.asInt() * 1000;
        dump("read timerLongPin: ", false);
        dump(timerLongValue);
}

BLYNK_WRITE(timerLongButtonPin){
        isTimerLongButtonPressed = param.asInt();
        dump("read timerLongButtonPin: ", false);
        dump(isTimerLongButtonPressed);
}

/**
 * @return bool
 */
bool isWifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

/**
 * @return bool
 */
bool isBlynkConnected() {
    return Blynk.connected() == true;
}

/**
 * @return bool
 */
bool isLedOn() {
    return digitalRead(LED_PIN) == HIGH;
}

/**
 * @return void
 */
void turnLedOn() {
    dump("turn LED on ", false);
    if (isLedOn() == false) {
        dump("yes");
        digitalWrite(LED_PIN, HIGH);
    } else {
        dump("no");
    }
}

/**
 * @return void
 */
void turnLedOff() {
    dump("turn LED off ", false);
    if (isLedOn() == true) {
        dump("yes");
        digitalWrite(LED_PIN, LOW);
    } else {
        dump("no");
    }
}

/**
 * @return void
 */
void syncBlynk() {
    Blynk.syncVirtual(timerShortButtonPin);
    Blynk.syncVirtual(timerLongButtonPin);
}

/**
 * @return void
 */
void toggleLed() {
    dump("toggle LED");
    if (isLedOn() == false) {
        turnLedOn();
    } else {
        turnLedOff();
    }
}

bool isMillButtonPressed() {
    return isTimerShortButtonPressed == 1 || isTimerLongButtonPressed == 1;
}

/**
 * @param int time
 * @return void
 */
void triggerRelay(int time) {
    dump("Relay ", false);
    if (isMillButtonPressed() == false) {
        dump("no button pressed");
        return;
    }

    if (relayTriggeredTime == 0) {
        dump("turned ON");
        relayTriggeredTime = millis();
        isRelayTriggered = true;
        digitalWrite(RELAY_PIN, HIGH);
    }

    if (millis() - relayTriggeredTime >= time) {
        dump("turned OFF");
        relayTriggeredTime = 0;
        isRelayTriggered = false;
        digitalWrite(RELAY_PIN, LOW);
        Blynk.virtualWrite(timerLongButtonPin, 0);
        Blynk.virtualWrite(timerShortButtonPin, 0);
    }
}

void resetWifiConnectingStats() {
    wifiConnectingCount = 0;
    wifiStartedConnectingTime = 0;
    isInWifiConnectingMode = false;
}

bool isOfflineModeForced() {
    return millis() - machineStartTime >= 60000 && isWifiConnected() == false;
}


/**
 * @return void
 */
void connectWifi() {
    dump("TEST");
    if (isWifiConnected() == true) {
        resetWifiConnectingStats();
        return;
    }
    dump("Wifi not connected");
    if (isInWifiConnectingMode == false && wifiStartedConnectingTime == 0) {
        wifiStartedConnectingTime = millis();
        isInWifiConnectingMode = true;
    }

    wifiConnectingCount++;

    if (millis() - wifiStartedConnectingTime >= 5000) {
        dump("Start connecting to wifi with ssid ", false);
        dump(wifiSsid);
        WiFi.hostname(wifiHostname);
        WiFi.disconnect();
        WiFi.begin(wifiSsid, wifiPass);
        resetWifiConnectingStats();
    }

    if (wifiConnectingCount % 500 == 0) {
        toggleLed();
    }
}

/**
 * @return void
 */
void connectBlynk() {
    while (isBlynkConnected() == false) {
        dump("Blynk offline");
        turnLedOff();
        Blynk.connect(3000);
        int count = 1;
        while (isBlynkConnected() == false) {
            dump("Waiting for reconnect to blynk", false);
            dump(count);
            delay(300);
            toggleLed();
            if (count == 15) {
                return;
            }
            count++;
        }
    };
}

void handleOtaStuff() {
    ArduinoOTA.handle();
    ArduinoOTA.onStart([]() {
        timer1_disable();
    });
    ArduinoOTA.onError([](ota_error_t error) {
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    });
    ArduinoOTA.onEnd([]() {
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    });
}

void completeSetupWifiIsConnected(){
    if (isSetupComplete == true || isWifiConnected() == true){
        return;
    }
    dump("Resume SetUp");

//    Blynk.begin(blynkAuthKey, wifiSsid, wifiPass, blynkHost, blynkPort);
    connectBlynk();


}


/**
 * @return void
 */
void setup() {
    if (isDebug == true) {
        Serial.begin(115200);
    }

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    connectWifi();
    Blynk.config(blynkAuthKey, blynkHost, blynkPort);

    if (isOta) {
        ArduinoOTA.setHostname(otaHost);
        ArduinoOTA.setPassword(otaPass);
        ArduinoOTA.begin();
    }

    if (machineStartTime == 0) {
        machineStartTime = millis();
    }
}

/**
 * @return void
 */
void loop() {
    bool offlineMode = isOfflineModeForced();
    bool isWifiConnectedVar = isWifiConnected();

    if (isSetupComplete == false && isWifiConnectedVar == true){
        completeSetupWifiIsConnected();
    }

    if (isWifiConnectedVar || offlineMode == true) {
        if (isWifiConnectedVar) {
            dump("Wifi is connected");

            handleOtaStuff();

            if (isBlynkConnected()) {
                dump("Blynk is connected");
                Blynk.syncAll();
                Blynk.run();
                syncBlynk();
            } else {
                dump("Blynk is disconnected");
                connectBlynk();
            }
        }
        if (offlineMode) {
            dump("Wifi is no connected => OFFLINE MODE");
            connectWifi();
        }
        turnLedOff();


        dump("Button short: ", false);
        dump(isTimerShortButtonPressed);
        dump("Button long: ", false);
        dump(isTimerLongButtonPressed);
        if (isTimerLongButtonPressed == 1 && isTimerShortButtonPressed == 0) {
            (timerLongValue);
        }

        if (isTimerLongButtonPressed == 0 && isTimerShortButtonPressed == 1) {
            triggerRelay(timerShortValue);
        }

    } else {
        dump("Wifi is not connected");
        connectWifi();
    }
}