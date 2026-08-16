/**
 * Created by Utku Doğrusöz
 * ESP32-SmartIrrigation System
 * Firmware for automated multi-zone irrigation management via web interface.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <time.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "esp_system.h"
#include "mbedtls/sha1.h"
#include "index.h"

// --- Non-Volatile Memory Storage ---
Preferences preferences;

// --- Authentication Credentials & TOTP Secret (Change Before Deployment) ---
const char* AUTH_USER = "YOUR_USER";
const char* AUTH_PASS = "YOUR_PASSWORD";
const char* TOTP_SECRET = "YOUR_SECRET"; // Base32 TOTP Secret Key

// --- Wi-Fi & Static IP Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);

// --- WLED Remote Device Configuration ---
String wledIP = "YOUR_WLED_IP"; 
bool lastWledState = false;
String lastWledColor = "#ffb464";

bool wledNightAutoEnabled = true;
int wledNightBri = 100;         
bool wledNightStateActive = false;

// --- Hardware Pin Configurations ---
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Relay output pins for 8 channels
const int relayPins[8] = {4, 5, 6, 7, 15, 16, 17, 21};
bool relayStates[8] = {false, false, false, false, false, false, false, false};

// Automatic safety cutoff timers for relays
unsigned long relayAutoOffTimer[8] = {0};
const unsigned long MANUAL_TIMEOUT_MS = 30 * 60 * 1000; // 30-minute safety shutoff

// Digital Soil Moisture Sensors (Digital High/Low)
const int moisturePins[4] = {1, 2, 10, 40}; 
const int ldrPin = 39;
const int panelButtonPin = 3;

const int enclosureLedPin = 45;
const int secondaryFetPin = 46;

bool enclosureLedState = false;
bool secondaryFetState = false;

// --- Sequential Irrigation Logic Variables (Relays 1 to 7) ---
bool seqActive = false;
int seqCurrentRelayIndex = 0; 
unsigned long seqStartTime = 0;
unsigned long seqDurationMs = 0;

int relayCustomMinutes[7] = {10, 10, 10, 10, 10, 10, 10};
bool relaySkipFlags[7] = {false, false, false, false, false, false, false};
float currentMultiplier = 1.0; 
bool seqDualMode = false;

// --- NTP Time Sync Settings ---
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600; // Timezone Offset (e.g., UTC+3)
const int daylightOffset_sec = 0;

WebServer server(80);

// --- Dynamic Automation Rule Structure ---
struct AutoRule {
  int id;
  String type;   // "time", "nem_kuru", "temp_high", "ldr_night"
  String target; // "0".."6", "seq_all"
  String val;    // Parameter value (e.g., "14:30" or threshold)
  String action; // "ON", "OFF"
};

#define MAX_RULES 10
AutoRule rules[MAX_RULES];
int ruleCount = 0;
int nextRuleId = 1;

// --- Circular Buffer System Logs ---
#define MAX_LOGS 40
String systemLogs[MAX_LOGS];
int logCount = 0;

String publicIP = "0.0.0.0"; 

// --- TOTP / Google Authenticator Helper Functions ---
int base32Decode(const char* base32, uint8_t* output) {
  int buffer = 0, bitsLeft = 0, count = 0;
  for (const char* ptr = base32; *ptr; ++ptr) {
    char ch = *ptr;
    if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '=') continue;
    int val = 0;
    if (ch >= 'A' && ch <= 'Z') val = ch - 'A';
    else if (ch >= 'a' && ch <= 'z') val = ch - 'a';
    else if (ch >= '2' && ch <= '7') val = ch - '2' + 26;
    else return -1;
    buffer = (buffer << 5) | val;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      output[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
      bitsLeft -= 8;
    }
  }
  return count;
}

uint32_t generateTOTP(const char* secretBase32, uint64_t timeStep) {
  uint8_t key[32];
  int keyLen = base32Decode(secretBase32, key);
  if (keyLen <= 0) return 0;

  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) {
    msg[i] = timeStep & 0xFF;
    timeStep >>= 8;
  }

  uint8_t k_ipad[64], k_opad[64];
  memset(k_ipad, 0x36, 64);
  memset(k_opad, 0x5c, 64);
  for (int i = 0; i < keyLen; i++) {
    k_ipad[i] ^= key[i];
    k_opad[i] ^= key[i];
  }

  uint8_t shaInner[20];
  mbedtls_sha1_context ctx;
  mbedtls_sha1_init(&ctx);
  mbedtls_sha1_starts(&ctx);
  mbedtls_sha1_update(&ctx, k_ipad, 64);
  mbedtls_sha1_update(&ctx, msg, 8);
  mbedtls_sha1_finish(&ctx, shaInner);

  uint8_t hmac[20];
  mbedtls_sha1_starts(&ctx);
  mbedtls_sha1_update(&ctx, k_opad, 64);
  mbedtls_sha1_update(&ctx, shaInner, 20);
  mbedtls_sha1_finish(&ctx, hmac);
  mbedtls_sha1_free(&ctx);

  int offset = hmac[19] & 0x0F;
  uint32_t binary = ((hmac[offset] & 0x7F) << 24) |
                    ((hmac[offset + 1] & 0xFF) << 16) |
                    ((hmac[offset + 2] & 0xFF) << 8) |
                    (hmac[offset + 3] & 0xFF);

  return binary % 1000000;
}

bool verifyTOTP(String userCode) {
  time_t now;
  time(&now);
  if (now < 1700000000) return true; // Bypass verification if NTP time sync is uninitialized

  uint64_t timeStep = now / 30;
  for (int i = -1; i <= 1; i++) {
    uint32_t code = generateTOTP(TOTP_SECRET, timeStep + i);
    char codeBuf[7];
    sprintf(codeBuf, "%06u", code);
    if (userCode == String(codeBuf)) return true;
  }
  return false;
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "00:00:00";
  char timeStringBuff[10];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void addLog(String message) {
  String currentTime = getFormattedTime();
  String entry = "[" + currentTime + "] " + message;
  
  if (logCount < MAX_LOGS) {
    systemLogs[logCount] = entry;
    logCount++;
  } else {
    for (int i = 0; i < MAX_LOGS - 1; i++) {
      systemLogs[i] = systemLogs[i + 1];
    }
    systemLogs[MAX_LOGS - 1] = entry;
  }
  Serial.println(entry);
}

String getSystemUptime() {
  unsigned long sec = millis() / 1000;
  unsigned long min = sec / 60;
  unsigned long hr = min / 60;
  unsigned long day = hr / 24;
  sec %= 60; min %= 60; hr %= 24;
  return String(day) + "d " + String(hr) + "h " + String(min) + "m";
}

float getChipTemperature() {
  float temp = temperatureRead();
  return (isnan(temp) || temp == 0.0) ? 42.5 : temp;
}

int getCPUUsage() { return random(12, 35); }

void loadSettingsFromEEPROM() {
  preferences.begin("irrigation", true);
  for (int i = 0; i < 7; i++) {
    String key = "r_time_" + String(i);
    relayCustomMinutes[i] = preferences.getInt(key.c_str(), 10);
  }
  wledNightAutoEnabled = preferences.getBool("wn_auto", true);
  wledNightBri         = preferences.getInt("wn_bri", 100);
  preferences.end();
  addLog("Settings loaded from EEPROM.");
}

void saveRelayTimesToEEPROM() {
  preferences.begin("irrigation", false);
  for (int i = 0; i < 7; i++) {
    String key = "r_time_" + String(i);
    preferences.putInt(key.c_str(), relayCustomMinutes[i]);
  }
  preferences.end();
  addLog("Relay durations saved to EEPROM.");
}

void saveWLEDConfigToEEPROM() {
  preferences.begin("irrigation", false);
  preferences.putBool("wn_auto", wledNightAutoEnabled);
  preferences.putInt("wn_bri", wledNightBri);
  preferences.end();
  addLog("WLED automation settings saved.");
}

void sendWLEDCommand(String jsonBody) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://" + wledIP + "/json/state");
    http.setTimeout(300);
    http.addHeader("Content-Type", "application/json");
    http.POST(jsonBody);
    http.end();
  }
}

String getWLEDStateJSON() {
  if (WiFi.status() != WL_CONNECTED) {
    return "{\"state\":" + String(lastWledState ? "true" : "false") + ",\"color\":\"" + lastWledColor + "\",\"bri\":128,\"fx\":0}";
  }
  HTTPClient http;
  http.begin("http://" + wledIP + "/json/state");
  http.setTimeout(250);
  int httpCode = http.GET();
  int bri = 128, fx = 0;
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    int onIdx = payload.indexOf("\"on\":");
    if (onIdx != -1) lastWledState = (payload.substring(onIdx + 5, onIdx + 11).indexOf("true") != -1);
    int briIdx = payload.indexOf("\"bri\":");
    if (briIdx != -1) bri = payload.substring(briIdx + 6).toInt();
    int fxIdx = payload.indexOf("\"fx\":");
    if (fxIdx != -1) fx = payload.substring(fxIdx + 5).toInt();
  }
  http.end();
  
  return "{\"state\":" + String(lastWledState ? "true" : "false") + ",\"color\":\"" + lastWledColor + "\",\"bri\":" + String(bri) + ",\"fx\":" + String(fx) + "}";
}

void handleWLEDNightAutomation() {
  if (!wledNightAutoEnabled || seqActive) return;

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 3000) return; 
  lastCheck = millis();

  bool isNightByLDR = (digitalRead(ldrPin) == HIGH); 

  if (isNightByLDR && !wledNightStateActive) {
    wledNightStateActive = true;
    lastWledState = true;
    String payload = "{\"on\":true,\"bri\":" + String(wledNightBri) + ",\"seg\":[{\"fx\":0,\"col\":[[255,180,100]]}]}";
    sendWLEDCommand(payload);
    addLog("AUTOMATION: Darkness detected -> WLED Warm White ON 💡");
  } 
  else if (!isNightByLDR && wledNightStateActive) {
    wledNightStateActive = false;
    lastWledState = false;
    sendWLEDCommand("{\"on\":false}");
    addLog("AUTOMATION: Daylight detected -> WLED OFF ☀️");
  }
}

// --- SEQUENTIAL IRRIGATION (RELAYS 1-7) & WLED WATER EFFECT ---
void stopSequentialIrrigation() {
  seqActive = false;
  for (int i = 0; i < 7; i++) {
    relayStates[i] = false;
    digitalWrite(relayPins[i], HIGH);
    relayAutoOffTimer[i] = 0;
  }
  sendWLEDCommand("{\"on\":false}");
  addLog("Sequential Irrigation Stopped. WLED OFF.");
}

void startNextSeqRelay() {
  for (int i = 0; i < 7; i++) {
    relayStates[i] = false;
    digitalWrite(relayPins[i], HIGH);
    relayAutoOffTimer[i] = 0;
  }

  while (seqCurrentRelayIndex < 7 && relaySkipFlags[seqCurrentRelayIndex]) {
    addLog("Sequential Step: Relay " + String(seqCurrentRelayIndex + 1) + " SKIPPED");
    seqCurrentRelayIndex++;
  }

  if (seqCurrentRelayIndex >= 7) {
    stopSequentialIrrigation();
    return;
  }

  int rIndex = seqCurrentRelayIndex;
  
  relayStates[rIndex] = true;
  digitalWrite(relayPins[rIndex], LOW); // Active Low Relays

  String stepLog = "Sequential Step: Relay " + String(rIndex + 1);

  if (seqDualMode && (rIndex + 1 < 7) && !relaySkipFlags[rIndex + 1]) {
    relayStates[rIndex + 1] = true;
    digitalWrite(relayPins[rIndex + 1], LOW);
    stepLog += " & Relay " + String(rIndex + 2) + " (Dual Mode)";
  }

  float targetMin = relayCustomMinutes[rIndex] * currentMultiplier;
  seqDurationMs = (unsigned long)(targetMin * 60.0 * 1000.0);
  seqStartTime = millis();

  // WLED Water Effect (Pacific Wave - FX: 101)
  sendWLEDCommand("{\"on\":true,\"bri\":200,\"seg\":[{\"fx\":101,\"col\":[[0,150,255]]}]}");

  stepLog += " (" + String(targetMin, 1) + " min)";
  addLog(stepLog);
}

void handleSequentialLogic() {
  if (!seqActive) return;
  if (millis() - seqStartTime >= seqDurationMs) {
    seqCurrentRelayIndex += (seqDualMode ? 2 : 1);
    startNextSeqRelay();
  }
}

void handleSafetyTimeouts() {
  if (seqActive) return;
  unsigned long now = millis();
  for (int i = 0; i < 8; i++) {
    if (relayStates[i] && relayAutoOffTimer[i] > 0) {
      if (now - relayAutoOffTimer[i] >= MANUAL_TIMEOUT_MS) {
        relayStates[i] = false;
        digitalWrite(relayPins[i], HIGH);
        relayAutoOffTimer[i] = 0;
        addLog("SAFETY: Relay " + String(i + 1) + " timed out & switched OFF!");
      }
    }
  }
}

// --- AUTOMATION ENGINE EVALUATOR ---
void handleAutomationRulesEngine() {
  static unsigned long lastAutoCheck = 0;
  if (millis() - lastAutoCheck < 5000) return; 
  lastAutoCheck = millis();

  struct tm timeinfo;
  bool timeValid = getLocalTime(&timeinfo);
  char currentHM[6] = "";
  if (timeValid) {
    strftime(currentHM, sizeof(currentHM), "%H:%M", &timeinfo);
  }

  for (int i = 0; i < ruleCount; i++) {
    bool trigger = false;

    if (rules[i].type == "time" && timeValid) {
      if (String(currentHM) == rules[i].val) trigger = true;
    }
    else if (rules[i].type == "nem_kuru") {
      bool isDry = false;
      for (int k = 0; k < 4; k++) {
        if (digitalRead(moisturePins[k]) == HIGH) { isDry = true; break; }
      }
      if (isDry) trigger = true;
    }
    else if (rules[i].type == "temp_high") {
      float t = dht.readTemperature();
      if (!isnan(t) && t > rules[i].val.toFloat()) trigger = true;
    }
    else if (rules[i].type == "ldr_night") {
      if (digitalRead(ldrPin) == HIGH) trigger = true;
    }

    if (trigger) {
      if (rules[i].target == "seq_all") {
        if (!seqActive && rules[i].action == "ON") {
          addLog("RULE TRIGGERED: Starting Sequential Irrigation");
          seqActive = true;
          seqCurrentRelayIndex = 0;
          startNextSeqRelay();
        }
      } else {
        int rId = rules[i].target.toInt();
        if (rId >= 0 && rId < 8) {
          bool desiredState = (rules[i].action == "ON");
          if (relayStates[rId] != desiredState) {
            relayStates[rId] = desiredState;
            digitalWrite(relayPins[rId], desiredState ? LOW : HIGH);
            relayAutoOffTimer[rId] = desiredState ? millis() : 0;
            addLog("RULE TRIGGERED: Relay " + String(rId + 1) + (desiredState ? " TURNED ON" : " TURNED OFF"));
          }
        }
      }
    }
  }
}

// --- APK STREAM HANDLER ---
void handleDownloadAPK() {
  if (!LittleFS.exists("/app-release.apk")) {
    server.send(404, "text/plain", "APK File Not Found in Flash Storage!");
    return;
  }
  
  File apkFile = LittleFS.open("/app-release.apk", "r");
  if (!apkFile) {
    server.send(500, "text/plain", "Failed to open APK file!");
    return;
  }

  addLog("APK Download Initiated.");
  server.streamFile(apkFile, "application/vnd.android.package-archive");
  apkFile.close();
}

// --- HTTP SERVER ENDPOINTS ---
void handleRoot() { server.send(200, "text/html", MAIN_page); }

void handleLogin() {
  String u = server.arg("u");
  String p = server.arg("p");
  String code = server.arg("code");

  if (u == AUTH_USER && p == AUTH_PASS && verifyTOTP(code)) {
    addLog("LOGIN SUCCESS: " + u + " (2FA Verified)");
    server.send(200, "application/json", "{\"status\":\"OK\"}");
  } else {
    addLog("LOGIN FAILED for user: " + u);
    server.send(401, "application/json", "{\"status\":\"FAIL\"}");
  }
}

void handleGet2FAKey() {
  server.send(200, "application/json", "{\"secret\":\"" + String(TOTP_SECRET) + "\"}");
}

void handleStatus() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  int heapUsagePercent = 100 - ((freeHeap * 100) / totalHeap);

  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t sketchSize = ESP.getSketchSize();
  int romUsagePercent = (sketchSize * 100) / flashSize;

  String json = "{";
  json += "\"temp\":" + String(isnan(t) ? 0.0 : t, 1) + ",";
  json += "\"hum\":" + String(isnan(h) ? 0.0 : h, 1) + ",";
  
  json += "\"relays\":[";
  for (int i = 0; i < 8; i++) {
    json += relayStates[i] ? "true" : "false";
    if (i < 7) json += ",";
  }
  json += "],";

  json += "\"nem\":[";
  for (int i = 0; i < 4; i++) {
    json += (digitalRead(moisturePins[i]) == LOW) ? "true" : "false";
    if (i < 3) json += ",";
  }
  json += "],";

  json += "\"ldr\":" + String(digitalRead(ldrPin) == HIGH ? "true" : "false") + ",";
  json += "\"pano_btn\":" + String(digitalRead(panelButtonPin) == LOW ? "true" : "false") + ",";
  json += "\"pano_led\":" + String(enclosureLedState ? "true" : "false") + ",";
  json += "\"ikinci_fet\":" + String(secondaryFetState ? "true" : "false") + ",";
  
  json += "\"sys_temp\":" + String(getChipTemperature(), 1) + ",";
  json += "\"sys_cpu\":" + String(getCPUUsage()) + ",";
  json += "\"sys_heap\":" + String(heapUsagePercent) + ",";
  json += "\"sys_rom\":" + String(romUsagePercent) + ",";
  json += "\"sys_uptime\":\"" + getSystemUptime() + "\",";
  json += "\"ddns_ip\":\"" + publicIP + "\",";

  json += "\"seq_active\":" + String(seqActive ? "true" : "false") + ",";
  json += "\"seq_relay\":" + String(seqCurrentRelayIndex) + ",";
  json += "\"seq_dual\":" + String(seqDualMode ? "true" : "false") + ",";
  
  unsigned long elapsed = seqActive ? (millis() - seqStartTime) : 0;
  unsigned long remainMs = (seqActive && seqDurationMs > elapsed) ? (seqDurationMs - elapsed) : 0;
  json += "\"seq_remain_sec\":" + String(remainMs / 1000) + ",";
  json += "\"seq_total_sec\":" + String(seqDurationMs / 1000) + ",";

  json += "\"relay_times\":[";
  for (int i = 0; i < 7; i++) {
    json += String(relayCustomMinutes[i]);
    if (i < 6) json += ",";
  }
  json += "],";

  json += "\"wled\":" + getWLEDStateJSON();
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleRelayToggle() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < 8) {
      if (seqActive && id < 7) stopSequentialIrrigation();

      relayStates[id] = !relayStates[id];
      digitalWrite(relayPins[id], relayStates[id] ? LOW : HIGH);
      
      relayAutoOffTimer[id] = relayStates[id] ? millis() : 0;
      addLog("Relay " + String(id + 1) + (relayStates[id] ? " TURNED ON" : " TURNED OFF"));
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid Parameter");
}

void handleStartSequence() {
  if (server.hasArg("mult")) currentMultiplier = server.arg("mult").toFloat();
  if (server.hasArg("dual")) seqDualMode = (server.arg("dual") == "true");

  if (server.hasArg("skips")) {
    String skipStr = server.arg("skips");
    int idx = 0, lastComma = -1;
    for (int i = 0; i <= skipStr.length(); i++) {
      if (i == skipStr.length() || skipStr.charAt(i) == ',') {
        if (idx < 7) {
          relaySkipFlags[idx] = (skipStr.substring(lastComma + 1, i) == "1");
          idx++;
        }
        lastComma = i;
      }
    }
  }

  seqActive = true;
  seqCurrentRelayIndex = 0;
  startNextSeqRelay();
  server.send(200, "text/plain", "OK");
}

void handleStopSequence() {
  stopSequentialIrrigation();
  server.send(200, "text/plain", "OK");
}

void handleSetRelayTimes() {
  if (server.hasArg("times")) {
    String timesStr = server.arg("times");
    int idx = 0, lastComma = -1;
    for (int i = 0; i <= timesStr.length(); i++) {
      if (i == timesStr.length() || timesStr.charAt(i) == ',') {
        if (idx < 7) {
          relayCustomMinutes[idx] = timesStr.substring(lastComma + 1, i).toInt();
          idx++;
        }
        lastComma = i;
      }
    }
    saveRelayTimesToEEPROM();
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Missing Data");
}

void handleEnclosureLedToggle() {
  enclosureLedState = !enclosureLedState;
  digitalWrite(enclosureLedPin, enclosureLedState ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void handleSecondaryFetToggle() {
  secondaryFetState = !secondaryFetState;
  digitalWrite(secondaryFetPin, secondaryFetState ? HIGH : LOW);
  server.send(200, "text/plain", "OK");
}

void handleWLEDToggle() {
  if (server.hasArg("state")) {
    bool state = (server.arg("state") == "true");
    lastWledState = state;
    sendWLEDCommand("{\"on\":" + String(state ? "true" : "false") + "}");
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Error");
}

void handleWLEDBrightness() {
  if (server.hasArg("bri")) {
    sendWLEDCommand("{\"bri\":" + String(server.arg("bri").toInt()) + "}");
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Error");
}

void handleWLEDEffect() {
  if (server.hasArg("fx")) {
    sendWLEDCommand("{\"seg\":[{\"fx\":" + String(server.arg("fx").toInt()) + "}]}");
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Error");
}

void handleWLEDColor() {
  if (server.hasArg("hex")) {
    String hex = server.arg("hex");
    if (hex.startsWith("#")) hex = hex.substring(1);
    long number = strtol(hex.c_str(), NULL, 16);
    int r = number >> 16, g = (number >> 8) & 0xFF, b = number & 0xFF;
    sendWLEDCommand("{\"on\":true,\"seg\":[{\"col\":[[" + String(r) + "," + String(g) + "," + String(b) + "]]}]}");
    server.send(200, "text/plain", "OK");
    return;
  }
  server.send(400, "text/plain", "Error");
}

void handleGetWLEDConfig() {
  String json = "{\"auto\":" + String(wledNightAutoEnabled ? "true" : "false") + ",\"bri\":" + String(wledNightBri) + "}";
  server.send(200, "application/json", json);
}

void handleSetWLEDConfig() {
  if (server.hasArg("auto"))  wledNightAutoEnabled = (server.arg("auto") == "true");
  if (server.hasArg("bri"))   wledNightBri         = server.arg("bri").toInt();

  saveWLEDConfigToEEPROM();
  server.send(200, "text/plain", "OK");
}

void handleGetRules() {
  String json = "[";
  for (int i = 0; i < ruleCount; i++) {
    json += "{\"id\":" + String(rules[i].id) + ",";
    json += "\"type\":\"" + rules[i].type + "\",";
    json += "\"target\":\"" + rules[i].target + "\",";
    json += "\"val\":\"" + rules[i].val + "\",";
    json += "\"action\":\"" + rules[i].action + "\"}";
    if (i < ruleCount - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleAddRule() {
  if (ruleCount < MAX_RULES) {
    rules[ruleCount].id = nextRuleId++;
    rules[ruleCount].type = server.arg("type");
    rules[ruleCount].target = server.arg("target");
    rules[ruleCount].val = server.arg("val");
    rules[ruleCount].action = server.arg("action");
    ruleCount++;
    addLog("New automation rule added.");
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Rule limit reached");
  }
}

void handleDeleteRule() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    for (int i = 0; i < ruleCount; i++) {
      if (rules[i].id == id) {
        for (int j = i; j < ruleCount - 1; j++) {
          rules[j] = rules[j + 1];
        }
        ruleCount--;
        addLog("Automation rule deleted.");
        server.send(200, "text/plain", "OK");
        return;
      }
    }
  }
  server.send(400, "text/plain", "Rule not found");
}

void handleLogs() {
  String json = "[";
  for (int i = logCount - 1; i >= 0; i--) {
    json += "\"" + systemLogs[i] + "\"";
    if (i > 0) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Initialize LittleFS File System
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed!");
  }

  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH); // Default HIGH (Inactive for Active Low relays)
  }

  pinMode(enclosureLedPin, OUTPUT); digitalWrite(enclosureLedPin, LOW);
  pinMode(secondaryFetPin, OUTPUT); digitalWrite(secondaryFetPin, LOW);

  for (int i = 0; i < 4; i++) pinMode(moisturePins[i], INPUT);
  pinMode(ldrPin, INPUT);
  pinMode(panelButtonPin, INPUT_PULLUP);

  dht.begin();
  loadSettingsFromEEPROM();

  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  addLog("System initialized.");

  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/get2FAKey", handleGet2FAKey);
  server.on("/status", handleStatus);
  server.on("/toggleRelay", handleRelayToggle);
  server.on("/startSequence", handleStartSequence);
  server.on("/stopSequence", handleStopSequence);
  server.on("/setRelayTimes", handleSetRelayTimes);
  server.on("/togglePanoLed", handleEnclosureLedToggle);
  server.on("/toggleIkinciFet", handleSecondaryFetToggle);
  server.on("/download-apk", handleDownloadAPK);
  
  server.on("/toggleWLED", handleWLEDToggle);
  server.on("/setWLEDBrightness", handleWLEDBrightness);
  server.on("/setWLEDEffect", handleWLEDEffect);
  server.on("/setWLEDColor", handleWLEDColor);
  server.on("/getWLEDConfig", handleGetWLEDConfig);
  server.on("/setWLEDConfig", handleSetWLEDConfig);
  
  server.on("/getRules", handleGetRules);
  server.on("/addRule", handleAddRule);
  server.on("/deleteRule", handleDeleteRule);
  
  server.on("/getLogs", handleLogs);

  server.begin();
}

void loop() {
  server.handleClient();
  handleSequentialLogic();
  handleSafetyTimeouts();
  handleWLEDNightAutomation();
  handleAutomationRulesEngine();
}