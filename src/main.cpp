// IMPORTANT: Define modem model BEFORE including TinyGSM
#define TINY_GSM_MODEM_SIM7600

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// ============================================================
//   CONFIGURATION — fill in your details here
// ============================================================

// WiFi credentials (fallback)
#define WIFI_SSID        "Acity-Guest"
#define WIFI_PASSWORD    "password@acity"

// APN for MTN Ghana
#define APN              "internet"
#define APN_USER         ""
#define APN_PASS         ""

// Supabase credentials
#define SERVER_HOST   "diplomatic-alignment-production-ebb5.up.railway.app"
#define SERVER_URL    "https://diplomatic-alignment-production-ebb5.up.railway.app/api/location"
// Device ID
#define DEVICE_ID        "tracker_01"

// How often to send GPS data (milliseconds)
#define UPDATE_INTERVAL  10000  // 10 seconds

// ============================================================
//   PIN DEFINITIONS for LilyGO SIM7600E
// ============================================================
#define MODEM_TX         27
#define MODEM_RX         26
#define MODEM_PWRKEY     4
#define MODEM_POWER_ON   25
#define MODEM_RST        5
#define GPS_RX           23
#define GPS_TX           22

// ============================================================
//   GLOBALS
// ============================================================
HardwareSerial SerialAT(1);   // UART1 for SIM7600
HardwareSerial SerialGPS(2);  // UART2 for GPS

TinyGsm        modem(SerialAT);
TinyGsmClient  gsmClient(modem);
TinyGPSPlus    gps;

bool useLTE  = false;
bool useWiFi = false;

unsigned long lastUpdate = 0;

// ============================================================
//   FUNCTION: Power on the SIM7600 modem
// ============================================================
void modemPowerOn() {
  Serial.println("[LTE] Powering on modem...");
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_POWER_ON, HIGH);

  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(100);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(5000);
}

// ============================================================
//   FUNCTION: Try connecting via LTE
// ============================================================
bool connectLTE() {
  Serial.println("[LTE] Initialising modem...");
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.println("[LTE] Modem: " + modemInfo);

  Serial.println("[LTE] Waiting for network...");
  if (!modem.waitForNetwork(60000L)) {
    Serial.println("[LTE] Network failed");
    return false;
  }

  Serial.println("[LTE] Connecting to APN...");
  if (!modem.gprsConnect(APN, APN_USER, APN_PASS)) {
    Serial.println("[LTE] APN failed");
    return false;
  }

  Serial.println("[LTE] Connected! IP: " + modem.localIP().toString());
  return true;
}

// ============================================================
//   FUNCTION: Try connecting via WiFi
// ============================================================
bool connectWiFi() {
  Serial.println("[WiFi] Connecting to: " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());
    return true;
  }

  Serial.println("\n[WiFi] Failed to connect");
  return false;
}

// ============================================================
//   FUNCTION: Enable GPS on SIM7600E via AT commands
// ============================================================
void enableGPS() {
  Serial.println("[GPS] Powering on GNSS via AT command...");
  
  // Turn off first in case it was already on
  SerialAT.println("AT+CGPS=0");
  delay(2000);
  while (SerialAT.available()) Serial.write(SerialAT.read());

  // Turn on GPS
  SerialAT.println("AT+CGPS=1,1");
  delay(2000);
  while (SerialAT.available()) Serial.write(SerialAT.read());

  // Check GPS status
  SerialAT.println("AT+CGPS?");
  delay(1000);
  while (SerialAT.available()) Serial.write(SerialAT.read());

  // Get GPS info
  SerialAT.println("AT+CGPSINFO");
  delay(1000);
  while (SerialAT.available()) Serial.write(SerialAT.read());

  Serial.println("[GPS] GNSS init complete");
}

// ============================================================
//   FUNCTION: Test GPS via AT command directly
// ============================================================
void testGPS() {
  Serial.println("[GPS] Testing GNSS module...");
  
  SerialAT.println("AT+CGPS?");
  delay(1000);
  while (SerialAT.available()) Serial.write(SerialAT.read());

  SerialAT.println("AT+CGPSINFO");
  delay(1000);
  while (SerialAT.available()) Serial.write(SerialAT.read());


  // Check signal strength
  SerialAT.println("AT+CSQ");
  delay(1000);
  while (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }
}
// ============================================================
//   FUNCTION: Send GPS data to Supabase via LTE
// ============================================================
void sendViaLTE(float lat, float lng, float spd, float altitude, int satellites) {
  HttpClient http(gsmClient, SERVER_HOST, 3000);

  JsonDocument doc;
  doc["device_id"]  = DEVICE_ID;
  doc["latitude"]   = lat;
  doc["longitude"]  = lng;
  doc["speed"]      = spd;
  doc["altitude"]   = altitude;
  doc["satellites"] = satellites;

  String body;
  serializeJson(doc, body);

  Serial.println("[LTE] Sending: " + body);

  http.beginRequest();
  http.post("/api/location");
  http.sendHeader("Host",           SERVER_HOST);
  http.sendHeader("Content-Type",   "application/json");
  http.sendHeader("Content-Length", String(body.length()));
  http.beginBody();
  http.print(body);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String responseBody = http.responseBody();
  Serial.println("[LTE] Status: " + String(statusCode));
  Serial.println("[LTE] Response: " + responseBody);
}
// ============================================================
//   FUNCTION: Send GPS data to Supabase via WiFi
// ============================================================
void sendViaWiFi(float lat, float lng, float spd, float altitude, int satellites) {
  WiFiClient client;
  HttpClient http(client, SERVER_HOST, 3000);

  JsonDocument doc;
  doc["device_id"]  = DEVICE_ID;
  doc["latitude"]   = lat;
  doc["longitude"]  = lng;
  doc["speed"]      = spd;
  doc["altitude"]   = altitude;
  doc["satellites"] = satellites;

  String body;
  serializeJson(doc, body);

  Serial.println("[WiFi] Sending: " + body);

  http.beginRequest();
  http.post("/api/location");
  http.sendHeader("Host",           SERVER_HOST);
  http.sendHeader("Content-Type",   "application/json");
  http.sendHeader("Content-Length", String(body.length()));
  http.beginBody();
  http.print(body);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String responseBody = http.responseBody();
  Serial.println("[WiFi] Status: " + String(statusCode));
  Serial.println("[WiFi] Response: " + responseBody);
}
// ============================================================
//   SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== GPS Tracker Starting ===");

  // Start GPS
  SerialGPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("[GPS] Serial started");

  // Try LTE first
  modemPowerOn();
  if (connectLTE()) {
    useLTE = true;
    Serial.println("[NET] Using LTE");
  } else {
    // Fall back to WiFi
    Serial.println("[NET] Falling back to WiFi...");
    if (connectWiFi()) {
      useWiFi = true;
      Serial.println("[NET] Using WiFi");
    } else {
      Serial.println("[NET] No connection available!");
    }
  }
enableGPS();
testGPS();
}
// ============================================================
//   LOOP
// ============================================================
void loop() {
  // Request GPS info from modem via AT command
  SerialAT.println("AT+CGPSINFO");
  delay(1000);

  String response = "";
  while (SerialAT.available()) {
    response += (char)SerialAT.read();
  }

  Serial.println("[MODEM] " + response);

  // Parse the +CGPSINFO response
  if (response.indexOf("+CGPSINFO:") >= 0 && response.indexOf(",,,,") == -1) {
    
    // Extract the data line
    int start = response.indexOf("+CGPSINFO:") + 10;
    String data = response.substring(start);
    data.trim();

    // Split by comma
    int idx = 0;
    String parts[10];
    for (int i = 0; i < 10; i++) {
      int comma = data.indexOf(',', idx);
      if (comma == -1) {
        parts[i] = data.substring(idx);
        break;
      }
      parts[i] = data.substring(idx, comma);
      idx = comma + 1;
    }

    // Convert NMEA to decimal degrees
    // Latitude: DDMM.MMMMMM
    float rawLat = parts[0].toFloat();
    int latDeg   = (int)(rawLat / 100);
    float latMin = rawLat - (latDeg * 100);
    float lat    = latDeg + (latMin / 60.0);
    if (parts[1] == "S") lat = -lat;

    // Longitude: DDDMM.MMMMMM
    float rawLng = parts[2].toFloat();
    int lngDeg   = (int)(rawLng / 100);
    float lngMin = rawLng - (lngDeg * 100);
    float lng    = lngDeg + (lngMin / 60.0);
    if (parts[3] == "W") lng = -lng;

    float speed    = parts[7].toFloat(); // speed in km/h
    float altitude = parts[6].toFloat(); // altitude in metres

    Serial.printf("[GPS] Lat: %.6f, Lng: %.6f, Speed: %.1f km/h, Alt: %.1f m\n",
                  lat, lng, speed, altitude);

    // Send to Supabase
    if (useLTE) {
      sendViaLTE(lat, lng, speed, altitude, 0);
    } else if (useWiFi) {
      sendViaWiFi(lat, lng, speed, altitude, 0);
    } else {
      Serial.println("[NET] No connection available");
    }

  } else {
    Serial.println("[GPS] No fix yet...");
  }

  // Wait before next update
  delay(UPDATE_INTERVAL);
}