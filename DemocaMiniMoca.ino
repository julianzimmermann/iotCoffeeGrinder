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

// This function will run every time Blynk connection is established
BLYNK_CONNECTED() {
    // Request Blynk server to re-send latest values for all pins
    Blynk.syncAll();

    // You can also update individual virtual pins like this:
    //Blynk.syncVirtual(V0, V2);

    // Let's write your hardware uptime to Virtual Pin 2
    int value = millis() / 1000;
    Blynk.virtualWrite(V2, value);
}

BLYNK_WRITE(V1)
        {
                int pinValue = param.asInt();
        }

void connectWifi() {
    WiFi.hostname(wifiHostname);
    do {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.disconnect();
            WiFi.begin(wifiSsid, wifiPass);
            while (WiFi.status() != WL_CONNECTED) {
                delay(100);
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
    Serial.begin(9600);

    Blynk.begin(blynkAuthKey, wifiSsid, wifiPass, blynkHost, blynkPort);

    if (isOta && WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.setHostname(otaHost);
        ArduinoOTA.setPassword(otaPass);
        ArduinoOTA.begin();
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {

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
    } else {
        connectWifi();
    }
}