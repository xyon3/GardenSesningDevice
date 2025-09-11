#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "dhtutils.h"

#define SENSOR_ID "50af716f"

#define AP_SSID "plantguard_50af716f"
#define AP_PASS "dhtsensor"

struct SenseData {
  double tmp; // temperature
  double rhu; // relative humidity
};

SenseData retrieveDataFromSensor();

void actionRegisterDhtSensor(String ip, String deviceName);

void actionSendSensorDataViaHTTP(double temperature, double relativeHumidity);


ESP8266WebServer server(80);
WiFiClientSecure client;


String scannedNetworks = "";
// String serverUrl = "http://192.168.18.76:3000/api/xyon3API/";
String serverUrl = "https://garden-sensing.vercel.app/api/xyon3API/";

// States
unsigned int isActive = 0;
String deviceIP = "";


SenseData retrieveDataFromSensor() {
  SenseData senseData;

  senseData.tmp = 23.2;
  senseData.rhu = 64;
  
  return senseData;
}

void actionRegisterDhtSensor(String ip, String deviceName) {
    HTTPClient http;
    http.begin(client, serverUrl + "device");
    http.addHeader("Content-Type", "application/json");


    // JSON body (replace with your own structure)
    String requestBody = "";
    requestBody += "{\"ip4\":\"";
    requestBody += ip;
    requestBody += "\",\"dnm\":\"";
    requestBody += deviceName;
    requestBody += "\",\"sts\":";
    requestBody += isActive;
    requestBody += ",\"did\":\"";
    requestBody += SENSOR_ID;
    requestBody += "\"}";

    int httpCode = http.POST(requestBody); // Send POST request

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.printf("Response code: %d\n", httpCode);
      Serial.println("Response body:");
      Serial.println(payload);
    } else {
      Serial.printf("POST failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

  http.end();
}

void actionSendSensorDataViaHTTP(double temperature, double relativeHumidity) {
    HTTPClient http;
    http.begin(client, serverUrl + "data");
    http.addHeader("Content-Type", "application/json");

    // JSON body (replace with your own structure)
    String requestBody = "";
    requestBody += "{\"rhu\":";
    requestBody += String(relativeHumidity);
    requestBody += ",\"tmp\":";
    requestBody += String(temperature);
    requestBody += ",\"did\":\"";
    requestBody += SENSOR_ID;
    requestBody += "\"}";

    int httpCode = http.POST(requestBody); // Send POST request

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.printf("Response code: %d\n", httpCode);
      Serial.println("Response body:");
      Serial.println(payload);
    } else {
      Serial.printf("POST failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

  http.end();
}

void handlePingDevice() {
    String requestBody = "";
    requestBody += "{\"sts\":\"";
    requestBody += isActive;
    requestBody += "\",\"did\":\"";
    requestBody += SENSOR_ID;
    requestBody += "\"}";
  server.send(200, "application/json", requestBody);
}

void handleToggleActivivty() {
  isActive = !isActive;

  actionRegisterDhtSensor(deviceIP, SENSOR_ID);

  String requestBody = "";
  requestBody += "{\"sts\":\"";
  requestBody += isActive;
  requestBody += "\"}";
  server.send(200, "application/json", requestBody);
}




// Scan Wi-Fi networks
void handleScan() {
  int n = WiFi.scanNetworks();
  scannedNetworks = "";
  for (int i = 0; i < n; ++i) {
    scannedNetworks += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  String html = "<h1 style='color: #007bff; text-align: center; font-family: Arial, sans-serif;'> Connect PlantGuard Sensor to WiFi</h1>";
  html += "<form action='/connect' method='get' style='max-width: 400px; margin: 20px auto; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);'>";
  html += "<label style='display: block; margin-bottom: 8px; font-weight: bold;'>SSID:</label>";
  html += "<select name='ssid' style='width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 4px;'>" + scannedNetworks + "</select><br>";
  html += "<label style='display: block; margin-bottom: 8px; font-weight: bold;'>Password:</label>";
  html += "<input type='password' name='pass' style='width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 4px;'><br>";
  html += "<input type='submit' value='Connect' style='width: 100%; padding: 10px; background-color: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer;'>";
  html += "</form>";

  server.send(200, "text/html", html);
}

// Connect to selected Wi-Fi
void handleConnectWifi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    String key = server.arg("key");


    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    String html = "<h1>Connecting to " + ssid + "</h1>";
    html += "<p>Check Serial Monitor for status...</p>";
    server.send(200, "text/html", html);

    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
      delay(500);
      Serial.print(".");
      tries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      // Save ssid & pass in EEPROM

      deviceIP = WiFi.localIP().toString();

      actionRegisterDhtSensor(deviceIP, SENSOR_ID);
      writeDataROM(ssid, pass, key);

      Serial.println("\nConnected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect.");
      Serial.println(WiFi.status());
      ESP.restart();

    }

  } else {
    server.send(400, "text/plain", "Missing SSID or Password");
  }
}


void setup() {
  Serial.begin(9600);
  delay(100);

  // Get EEPROM data

  // Start as AP Mode if $padding$ is not found from EEPROM Data, else do handle connect.
  client.setInsecure();


  // Start as Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleScan);
  server.on("/connect", handleConnectWifi);

  server.on("/toggleActive", handleToggleActivivty);
  server.on("/ping", handlePingDevice);


  server.begin();
  Serial.println("Web server started");
}

unsigned long lastSensorRead = 0;
// const unsigned long interval = 5UL * 60UL * 1000UL; // 5 minutes

const unsigned long interval = 6000; // 6 seconds


void loop() {
  server.handleClient();

  if (isActive) {
  unsigned long now = millis();
  if (now - lastSensorRead >= interval) {
    lastSensorRead = now;


    // 🔹 Your sensor code here
    Serial.println("Reading sensor...");

    SenseData senseData = retrieveDataFromSensor();
    actionSendSensorDataViaHTTP(senseData.tmp, senseData.rhu);

    // e.g. float temp = readDHT22();retrieveDataFromSensor
    // send data to server, etc.
  }
  }


}
