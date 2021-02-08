#include <ArduinoOTA.h>
#include "config.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

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
int timerShortButtonPin = BLYNK_TIMER_SHORT_VIRTUAL_BUTTON_PIN;
int timerLongPin = BLYNK_TIMER_LONG_VIRTUAL_PIN;
int timerLongButtonPin = BLYNK_TIMER_LONG_VIRTUAL_BUTTON_PIN;
/* ///////////////// */

int timerShortValue = 0;
int isTimerShortButtonPressed = 0;
int timerLongValue = 0;
int isTimerLongButtonPressed = 0;

// This function will run every time Blynk connection is established
BLYNK_CONNECTED() {
    // Request Blynk server to re-send latest values for all pins
    Blynk.syncAll();

    // You can also update individual virtual pins like this:
    //Blynk.syncVirtual(V0, V2);

    // Let's write your hardware uptime to Virtual Pin 2
    int value = millis() / 1000;
    Blynk.virtualWrite(V10, value);
}

BLYNK_WRITE(timerShortPin){
    timerShortValue = param.asInt() / 1000;
}

BLYNK_WRITE(timerShortButtonPin){
    isTimerShortButtonPressed = param.asInt();
}

BLYNK_WRITE(timerLongPin){
    timerLongValue = param.asInt() / 1000;
}

BLYNK_WRITE(timerLongButtonPin){
    isTimerLongButtonPressed = param.asInt();
}

void ledOn(){
    if (digitalRead(LED_PIN) == LOW){
        digitalWrite(LED_PIN, HIGH);
    }
}

void ledOff(){
    if (digitalRead(LED_PIN) == HIGH){
        digitalWrite(LED_PIN, LOW);
    }
}
/**
 * @param time
 */
void triggerRelay(int time){
    digitalWrite(RELAY_PIN, HIGH);
    delay(3000);
    digitalWrite(RELAY_PIN, LOW);
    Blynk.virtualWrite(timerLongButtonPin, 0);
    Blynk.virtualWrite(timerShortButtonPin, 0);
}

void connectWifi() {
    WiFi.hostname(wifiHostname);
    do {
        if (WiFi.status() != WL_CONNECTED) {
            ledOff();
            WiFi.disconnect();
            WiFi.begin(wifiSsid, wifiPass);
            while (WiFi.status() != WL_CONNECTED) {
                ledOn();
                delay(1000);
                ledOff();
            }
        }
    } while (WiFi.status() != WL_CONNECTED);

}

void connectBlynk() {
    if (Blynk.connected() != 1) {
        Blynk.connect(3000);
    }
}


void setup() {
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    Blynk.begin(blynkAuthKey, wifiSsid, wifiPass, blynkHost, blynkPort);

    if (isOta && WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.setHostname(otaHost);
        ArduinoOTA.setPassword(otaPass);
        ArduinoOTA.begin();
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        ledOn();
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

        if (Blynk.connected()) {
            Blynk.run();
        } else {
            connectBlynk();
        }

        if (isTimerLongButtonPressed == 1 && isTimerShortButtonPressed == 0){
            triggerRelay(timerLongValue);
        }

        if (isTimerLongButtonPressed == 0 && isTimerShortButtonPressed == 1){
            triggerRelay(timerShortValue);
        }

    } else {
        connectWifi();
    }
}