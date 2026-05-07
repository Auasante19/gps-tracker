// IMPORTANT: Define modem model BEFORE including TinyGSM
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SSLClientESP32.h>
#include <sys/time.h>
#include <time.h>

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

static const char SERVER_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Device ID
#define DEVICE_ID        "tracker_01"

// How often to send GPS data (milliseconds)
#define UPDATE_INTERVAL  300000  // 5 minutes

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
SSLClientESP32 lteSslClient(&gsmClient);
TinyGPSPlus    gps;

bool useLTE  = false;
bool useWiFi = false;

unsigned long lastUpdate = 0;

bool hasValidSystemTime() {
  return time(nullptr) >= 1704067200;  // 2024-01-01T00:00:00Z
}

// ============================================================
//   FUNCTION: Convert UTC date to Unix epoch days
// ============================================================
long daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097L + (long)doe - 719468L;
}

// ============================================================
//   FUNCTION: Sync ESP32 clock from SIM7600 network time
// ============================================================
bool syncEspClockFromModem() {
  Serial.println("[TIME] Syncing modem clock via NTP...");
  byte ntpStatus = modem.NTPServerSync("pool.ntp.org", 0);
  Serial.println("[TIME] Modem NTP: " + modem.ShowNTPError(ntpStatus));

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  float timezone = 0;

  if (!modem.getNetworkTime(&year, &month, &day, &hour, &minute, &second, &timezone)) {
    Serial.println("[TIME] Could not read modem network time");
    return false;
  }

  if (year < 2024 || month < 1 || month > 12 || day < 1 || day > 31) {
    Serial.printf("[TIME] Ignoring invalid modem time: %04d-%02d-%02d %02d:%02d:%02d TZ %.2f\n",
                  year, month, day, hour, minute, second, timezone);
    return false;
  }

  long epoch = daysFromCivil(year, month, day) * 86400L +
               hour * 3600L + minute * 60L + second -
               (long)(timezone * 3600.0f);

  struct timeval tv;
  tv.tv_sec = (time_t)epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);

  Serial.printf("[TIME] ESP32 clock set: %04d-%02d-%02d %02d:%02d:%02d TZ %.2f\n",
                year, month, day, hour, minute, second, timezone);
  return true;
}

// ============================================================
//   FUNCTION: Sync ESP32 clock over WiFi
// ============================================================
bool syncEspClockFromWiFi() {
  if (hasValidSystemTime()) return true;

  Serial.println("[TIME] Syncing ESP32 clock over WiFi...");
  configTime(0, 0, "pool.ntp.org", "time.google.com");

  unsigned long deadline = millis() + 15000;
  while (millis() < deadline) {
    if (hasValidSystemTime()) {
      time_t now = time(nullptr);
      struct tm timeInfo;
      gmtime_r(&now, &timeInfo);
      Serial.printf("[TIME] WiFi clock set: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                    timeInfo.tm_year + 1900,
                    timeInfo.tm_mon + 1,
                    timeInfo.tm_mday,
                    timeInfo.tm_hour,
                    timeInfo.tm_min,
                    timeInfo.tm_sec);
      return true;
    }
    delay(500);
  }

  Serial.println("[TIME] WiFi clock sync timed out");
  return false;
}

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
  syncEspClockFromModem();
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
    syncEspClockFromWiFi();
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
//   FUNCTION: Send JSON over LTE using ESP32 mbedTLS
// ============================================================
bool postJsonViaLTETls(const char* path, const String& body, const char* label) {
  Serial.printf("[%s] Connecting TLS to %s:443\n", label, SERVER_HOST);

  lteSslClient.stop();
  lteSslClient.setClient(&gsmClient);
  lteSslClient.setCACert(SERVER_ROOT_CA);
  lteSslClient.setHandshakeTimeout(45);

  if (!lteSslClient.connect(SERVER_HOST, 443)) {
    char err[128] = {0};
    int code = lteSslClient.lastError(err, sizeof(err));
    Serial.printf("[%s] TLS connect failed: %d %s\n", label, code, err);
    lteSslClient.stop();
    return false;
  }

  Serial.printf("[%s] TLS connected, sending HTTPS POST %s\n", label, path);
  lteSslClient.print("POST ");
  lteSslClient.print(path);
  lteSslClient.print(" HTTP/1.1\r\n");
  lteSslClient.print("Host: ");
  lteSslClient.print(SERVER_HOST);
  lteSslClient.print("\r\n");
  lteSslClient.print("Content-Type: application/json\r\n");
  lteSslClient.print("Connection: close\r\n");
  lteSslClient.print("Content-Length: ");
  lteSslClient.print(body.length());
  lteSslClient.print("\r\n\r\n");
  lteSslClient.print(body);

  String response;
  unsigned long deadline = millis() + 30000;
  unsigned long lastByteAt = millis();

  while (millis() < deadline) {
    while (lteSslClient.available()) {
      char c = (char)lteSslClient.read();
      response += c;
      lastByteAt = millis();
    }

    if (!lteSslClient.connected() && !lteSslClient.available()) break;
    if (response.length() > 0 && millis() - lastByteAt > 3000) break;
    delay(10);
  }

  lteSslClient.stop();

  Serial.printf("[%s] HTTPS response:\n", label);
  Serial.println(response.length() ? response : "(no response)");

  int status = 0;
  int statusStart = response.indexOf("HTTP/1.");
  if (statusStart >= 0) {
    int codeStart = response.indexOf(' ', statusStart);
    if (codeStart >= 0) status = response.substring(codeStart + 1, codeStart + 4).toInt();
  }

  Serial.printf("[%s] HTTPS status: %d\n", label, status);
  return status >= 200 && status < 300;
}
// ============================================================
//   FUNCTION: Send GPS data to Supabase via LTE
// ============================================================
void sendViaLTE(float lat, float lng, float spd, float altitude, int satellites) {
  JsonDocument doc;
  doc["device_id"]  = DEVICE_ID;
  doc["latitude"]   = lat;
  doc["longitude"]  = lng;
  doc["speed"]      = spd;
  doc["altitude"]   = altitude;
  doc["satellites"] = satellites;

  String body;
  serializeJson(doc, body);

  Serial.println("[LTE] Sending via mbedTLS HTTPS: " + body);
  postJsonViaLTETls("/api/location", body, "LTE");
}
// ============================================================
//   FUNCTION: Send GPS data to Supabase via WiFi
// ============================================================
void sendViaWiFi(float lat, float lng, float spd, float altitude, int satellites) {
  syncEspClockFromWiFi();

  WiFiClientSecure client;
  client.setCACert(SERVER_ROOT_CA);

  HttpClient http(client, "diplomatic-alignment-production-ebb5.up.railway.app", 443);

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
  http.sendHeader("Host",           "diplomatic-alignment-production-ebb5.up.railway.app");
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
//   FUNCTION: Scan WiFi and send for trilateration
// ============================================================
void sendWiFiTrilateration() {
  Serial.println("[WiFi] Starting WiFi scan for trilateration...");
  
  // Turn on WiFi just for scanning (even if we're using LTE for data)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  Serial.println("[WiFi] Scanning for anchor networks...");
  int networksFound = WiFi.scanNetworks();
  
  if (networksFound == 0) {
    Serial.println("[WiFi] No networks found");
    return;
  }

  Serial.println("[WiFi] Found " + String(networksFound) + " networks");

  // Build JSON with all found networks
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  JsonArray networks = doc["networks"].to<JsonArray>();

  int matchCount = 0;
  for (int i = 0; i < networksFound; i++) {
    String ssid = WiFi.SSID(i);
    int rssi    = WiFi.RSSI(i);

    // Only include our known anchor networks
    if (ssid == "MTN_4G_E09BD1" || ssid == "Tenda_030398") {
      JsonObject network = networks.add<JsonObject>();
      network["ssid"] = ssid;
      network["rssi"] = rssi;
      matchCount++;
      Serial.println("[WiFi] Found anchor: " + ssid + " RSSI: " + String(rssi));
    }
  }

  WiFi.scanDelete();

  if (matchCount == 0) {
    Serial.println("[WiFi] No anchor networks found in scan");
    return;
  }

  String body;
  serializeJson(doc, body);
  Serial.println("[WiFi] Sending trilateration data: " + body);

  // Send via LTE if available otherwise WiFi
  if (useLTE) {
    postJsonViaLTETls("/api/wifi-location", body, "WiFi-Tri");

  } else if (useWiFi) {
    // Reconnect to WiFi for sending
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi-Tri] Failed to reconnect for HTTPS POST");
      return;
    }

    syncEspClockFromWiFi();

    WiFiClientSecure client;
    client.setCACert(SERVER_ROOT_CA);
    HttpClient http(client, "diplomatic-alignment-production-ebb5.up.railway.app", 443);

    http.beginRequest();
    http.post("/api/wifi-location");
    http.sendHeader("Host",           "diplomatic-alignment-production-ebb5.up.railway.app");
    http.sendHeader("Content-Type",   "application/json");
    http.sendHeader("Content-Length", String(body.length()));
    http.beginBody();
    http.print(body);
    http.endRequest();

    int statusCode = http.responseStatusCode();
    Serial.println("[WiFi-Tri] Status: " + String(statusCode));
    Serial.println("[WiFi-Tri] Response: " + http.responseBody());
  }
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

  if (response.indexOf("+CGPSINFO:") >= 0 && response.indexOf(",,,,") == -1) {
    
    int start = response.indexOf("+CGPSINFO:") + 10;
    String data = response.substring(start);
    data.trim();

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
    float rawLat = parts[0].toFloat();
    int latDeg   = (int)(rawLat / 100);
    float latMin = rawLat - (latDeg * 100);
    float lat    = latDeg + (latMin / 60.0);
    if (parts[1] == "S") lat = -lat;

    float rawLng = parts[2].toFloat();
    int lngDeg   = (int)(rawLng / 100);
    float lngMin = rawLng - (lngDeg * 100);
    float lng    = lngDeg + (lngMin / 60.0);
    if (parts[3] == "W") lng = -lng;

    float speed    = parts[7].toFloat();
    float altitude = parts[6].toFloat();

    Serial.printf("[GPS] Lat: %.6f, Lng: %.6f, Speed: %.1f km/h, Alt: %.1f m\n",
                  lat, lng, speed, altitude);

    // GPS fix available — send via LTE or WiFi
    if (useLTE) {
      sendViaLTE(lat, lng, speed, altitude, 0);
    } else if (useWiFi) {
      sendViaWiFi(lat, lng, speed, altitude, 0);
    }

  } else {
    // No GPS fix — fall back to WiFi trilateration
    Serial.println("[GPS] No fix — trying WiFi trilateration...");
    sendWiFiTrilateration();
  }

  delay(UPDATE_INTERVAL);
}
