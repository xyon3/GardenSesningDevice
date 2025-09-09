#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "dhtutils.h"

#define SENSOR_ID "50af716f"

#define AP_SSID "dht_50af716f"
#define AP_PASS "dhtsensor"

ESP8266WebServer server(80);
WiFiClient client;

String scannedNetworks = "";
String serverUrl = "http://192.168.18.76:3000/api/xyon3API/device";

void registerDhtSensor(String ip, String deviceName) {
    HTTPClient http;
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");


    // JSON body (replace with your own structure)
    String requestBody = "";
    requestBody += "{\"ip4\":\"";
    requestBody += ip;
    requestBody += "\",\"dnm\":\"";
    requestBody += deviceName;
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


// Scan Wi-Fi networks
void handleScan() {
  int n = WiFi.scanNetworks();
  scannedNetworks = "";
  for (int i = 0; i < n; ++i) {
    scannedNetworks += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  String html = "<h1 style='color: #007bff; text-align: center; font-family: Arial, sans-serif;'>(dhtsensing) Connect to Wifi</h1>";
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
      writeDataROM(ssid, pass, key);
      registerDhtSensor(WiFi.localIP().toString(), SENSOR_ID);

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
  Serial.begin(115200);
  delay(100);

  // Get EEPROM data

  // Start as AP Mode if $padding$ is not found from EEPROM Data, else do handle connect.


  // Start as Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleScan);
  server.on("/connect", handleConnectWifi);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
