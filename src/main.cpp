#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "ArduinoJson.hpp"

//const unsigned delay_led = 20;
//const int step = 300;
//int led_PWM = -step;
//unsigned led_time = 0;
//unsigned button_time = 0;
//const unsigned delay_button = 100;
//
//bool button_pressed = false;
//bool prev_button_pressed = false;
//
//void setup() {
//    Serial.begin(9600);
//    pinMode(2, OUTPUT);
//    pinMode(5, INPUT_PULLUP);
//}
//
//void loop() {
//    if (millis() - led_time >= delay_led) {
//        led_time = millis();
//
//        if (led_PWM >= 0) {
//            analogWrite(2, led_PWM);
//
//            if (led_PWM > step) {
//                led_PWM = -step;
//            }
//        } else {
//            analogWrite(2, -led_PWM);
//        }
//        ++led_PWM;
//    }
//
//    bool pressed = digitalRead(5);
//    if(!prev_button_pressed && pressed)
//    {
//        if((millis() - button_time >= delay_button))
//        {
//            button_time = millis();
//            Serial.println("Aboba");
//        }
//    }
//    prev_button_pressed = pressed;
//}

char const *ssid = "gabe phone";
char const *passwd = "08121957";

unsigned timer_delay = 5000;
unsigned last_time = 0;

HTTPClient httpClient;

String city = "499161";
String countryCode = "RU";
String openWeatherMapApiKey = "d5c5f6913f4350bfb4c6545e24c343ed";
String jsonBuffer;
WiFiClient client;
String lon = "46.476916";
String lat = "41.528801";

void setup() {
    Serial.begin(9600);
    WiFi.begin(ssid, passwd);
    Serial.println("Connecting...");

    int i = 1;
    for (; i <= 5 || WiFi.status() != WL_CONNECTED; ++i) {
        Serial.println(i);
        delay(1000);
    }

    if (i == 5) {
        Serial.println("WiFi connection error");
    } else {
        Serial.println("WiFi connection success");
    }

    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    client.connect("http://api.openweathermap.org", 80);

    String serverPath = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&appid="+openWeatherMapApiKey;

    httpClient.begin(client, serverPath);
    if (httpClient.connected()) {
        Serial.println("HTTP success");
    } else {
        Serial.println("HTTP error");
    }
}

void loop() {
    if (millis() - last_time >= timer_delay) {
        last_time = millis();
        int code = httpClient.GET();
        if (code == 200) {
            String json = httpClient.getString();
            Serial.println(json);
        } else {
            Serial.print("GET ERROR ");
            Serial.println(code);
        }
    }
}