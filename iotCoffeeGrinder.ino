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
    }
    else{
        dump("no");
    }
}

/**
 * @return void
 */
void turnLedOff() {
    dump("turn LED off ", false);
    if (isLedOn()) {
        dump("yes");
        digitalWrite(LED_PIN, LOW);
    }
    else{
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

/**
 * @param int time
 * @return void
 */
void triggerRelay(int time) {
    Blynk.virtualWrite(V30, time);
    toggleLed();
    delay(500);
    toggleLed();
    Blynk.virtualWrite(timerLongButtonPin, 0);
    Blynk.virtualWrite(timerShortButtonPin, 0);
}

/**
 * @return void
 */
void connectWifi() {
    WiFi.hostname(wifiHostname);
    unsigned long startTime = millis();
    while (isWifiConnected() == false) {
        dump("Wifi not connected");
        turnLedOff();
        WiFi.disconnect();
        WiFi.begin(wifiSsid, wifiPass);
        int count = 1;
        while (isWifiConnected() == false) {
            delay(500);
            dump("Waiting for reconnect ", false);
            dump(count);
            toggleLed();
            if (count == 5){
                delay(2000);
                return;
            }
            count++;
        }
    };
}

/**
 * @return void
 */
void connectBlynk() {
    while (isBlynkConnected() == false) {
        dump("Blynk offline");
        turnLedOff();
        Blynk.connect(200);
        int count = 1;
        while (isBlynkConnected() == false) {
            dump("Waiting for reconnect to blynk", false);
            dump(count);
            delay(300);
            toggleLed();
            if (count == 15){
                return;
            }
            count++;
        }
    };
}


/**
 * @return void
 */
void setup() {
    if (isDebug == true){
        Serial.begin(115200);
    }

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    Blynk.begin(blynkAuthKey, wifiSsid, wifiPass, blynkHost, blynkPort);

    if (isOta && isWifiConnected()) {
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
    if (isWifiConnected()) {
        dump("Wifi is connected");
        turnLedOff();
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

        if (isBlynkConnected()) {
            dump("Blynk is connected");
            Blynk.syncAll();
            Blynk.run();
            syncBlynk();
        } else {
            dump("Blynk is disconnected");
            connectBlynk();
        }

        dump("Button short: ", false);
        dump(isTimerShortButtonPressed);
        dump("Button long: ", false);
        dump(isTimerLongButtonPressed);
        if (isTimerLongButtonPressed == 1 && isTimerShortButtonPressed == 0) {
            triggerRelay(timerLongValue);
        }

        if (isTimerLongButtonPressed == 0 && isTimerShortButtonPressed == 1) {
            triggerRelay(timerShortValue);
        }

    } else {
        dump("Wifi is not connected");
        connectWifi();
    }
}