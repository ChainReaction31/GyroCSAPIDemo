#include <Arduino.h>
#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"
#include <Wire.h>
#include <secrets.h>
#include <ArduinoJson.h>
#include <ctime>
#include <NTPClient.h>
#include <RTC.h>
#include <WiFiUdp.h>
#include "main.h"

// WIFI
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;
//WiFiServer server(80);
WiFiClient client;
int status = WL_IDLE_STATUS;

// Target Node
IPAddress server(34, 67, 197, 57);
const char *host = "http://34.67.197.57";
const char *api_root = "/sensorhub/api";
String api_endpoint = String(host) + ":8585" + String(api_root);
String obs_endpoint = "/datastreams/hjdvcjp6an6ie/observations";
//String observations_url = String(host) + String(api_root) + "/datastreams/hjdvcjp6an6ie/observations";
char api_user[] = SECRET_UNAME;
char api_pass[] = SECRET_UPASS;
String auth = SECRET_B64;


// GY-521
const int GY_ADDR = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
const int ACCL_CONV = 16384;
const int GYRO_CONV = 131;
// Button
// Tilt-switch

// RTC Stuff
WiFiUDP udp;
NTPClient timeClient(udp);

// JSON Data
DynamicJsonDocument doc(1024);

ArduinoLEDMatrix matrix;
const uint32_t happy[] = {
        0x19819,
        0x80000001,
        0x81f8000
};

const uint32_t heart[] = {
        0x3184a444,
        0x44042081,
        0x100a0040
};


void setup() {
    // Gyro
    Wire.begin();
    Wire.beginTransmission(GY_ADDR);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);

    Serial.begin(9600);
    while (!Serial) { ; // wait for serial port to connect.
    }
    matrix.begin();

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);

        // Connect to WPA/WPA2 network.
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    printWifiStatus();

    // Sync time
    RTC.begin();
    timeClient.begin();
    timeClient.update();
    auto unix_time = timeClient.getEpochTime();
    Serial.println(unix_time);
    RTCTime timeToSet = RTCTime(unix_time);
    RTC.setTime(timeToSet);

    RTCTime currentTime;
    RTC.getTime(currentTime);
    Serial.println("RTC was set to: " + String(currentTime));

}

void loop() {
// write your code here
    matrix.loadFrame(happy);
    delay(1000);
    matrix.loadFrame(heart);
    delay(1000);

    RTCTime currentTime;
    // Get current time from RTC
    RTC.getTime(currentTime);
    //Unix timestamp

    Wire.beginTransmission(GY_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(GY_ADDR, 12, true);
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();

    sendJsonData(GyX / GYRO_CONV, GyY / GYRO_CONV, GyZ / GYRO_CONV, AcX / ACCL_CONV, AcY / ACCL_CONV, AcZ / ACCL_CONV);


    delay(333);
}

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void sendJsonData(float x, float y, float z, float x2, float y2, float z2) {

//    Serial.print("Accelerometer: ");
//    Serial.print("X = ");
//    Serial.print(x2);
//    Serial.print(" | Y = ");
//    Serial.print(y2);
//    Serial.print(" | Z = ");
//    Serial.println(z2);
//
//    Serial.print("Gyroscope: ");
//    Serial.print("X  = ");
//    Serial.print(x);
//    Serial.print(" | Y = ");
//    Serial.print(y);
//    Serial.print(" | Z = ");
//    Serial.println(z);
//    Serial.println(" ");

    RTCTime currentTime;
    RTC.getTime(currentTime);
    String curr_time_str = String(currentTime) + "Z";
    Serial.println("Current Time: " + curr_time_str);
    int unix_time = currentTime.getUnixTime();
    Serial.println("Current Unix Time: " + String(unix_time));
    doc["phenomenonTime"] = curr_time_str;
    doc["resultTime"] = curr_time_str;
    doc["result"]["timestamp"] = unix_time;
    doc["result"]["gyro-readings"]["x"] = x;
    doc["result"]["gyro-readings"]["y"] = y;
    doc["result"]["gyro-readings"]["z"] = z;
    doc["result"]["accelerometer-readings"]["x"] = x2;
    doc["result"]["accelerometer-readings"]["y"] = y2;
    doc["result"]["accelerometer-readings"]["z"] = z2;

    // Send the HTTP POST request
    if (client.connect(server, 8585)) {
        Serial.println("Connected to server");
        // Make a HTTP request:
        client.println("POST " + String(api_root) + obs_endpoint + " HTTP/1.1");
        client.println("Host: " + String(host) + String(":8585"));
        client.println("Authorization: Basic " + auth);
        client.println("Content-Type: application/json");
        client.println("Content-Length: " + String(measureJson(doc)));
        client.println("Connection: close");
        client.println();
        serializeJson(doc, client);
        client.println();

    } else {
        // if you didn't get a connection to the server:
        Serial.println("Connection failed");
    }
}
