#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include "ArduinoJson.hpp"
#include "IOTConnection.hpp"

// char const *ssid = "gabe phone";
// char const *passwd = "08121957";

char const *ssid = "GabeNet2G";
char const *passwd = "17072003";

unsigned timer_delay = 5000;
unsigned last_time = 0;

//HTTPClient httpClient;
//
//void setupSleepHttpMode()
//{
//    httpClient.connect("http://" + ipAddress, port);
//    httpClient.begin(client, "http://127.0.0.1/" + String(id));
//
//    if (httpClient.connected()) {
//        Serial.println("HTTP connection success");
//    } else {
//        Serial.println("HTTP connection error");
//    }
//
//    int code = httpClient.GET();
//    if (code == 200) {
//        String json = httpClient.getString();
//        Serial.println(json);
//    } else {
//        Serial.print("GET ERROR ");
//        Serial.println(code);
//    }
//}

void setupWebsocketsMode()
{

}

IOTConnection connection;

void setup() {
    Serial.begin(9600);
    connection.connect("ESP32_TEST");
}

void loop() {
    connection.loop();
    if (millis() - last_time >= timer_delay) {
    }
}