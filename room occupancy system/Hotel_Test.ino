#include <WiFi.h>
#include <time.h>
#include "FS.h"
#include "SPIFFS.h"
#include "LD2410B.h"
#include <FirebaseESP32.h>
#include <WiFiManager.h>
WiFiManager wm;
// NTP config
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 6 * 3600;  // Bangladesh GMT+6
const int   daylightOffset_sec = 0;

// Firebase config
#define DATABASE_URL "https://hotel-monitor-ada02-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define DATABASE_SECRET "your_firebase_database_secret"

LD2410B radar(Serial2);

String currentDate = "";
File currentFile;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

RTC_DATA_ATTR int bootCount = 0;

void updateLogFile() {
  if (currentFile) currentFile.close();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Time not available");
    return;
  }

  char dateStr[11];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
  currentDate = String(dateStr);

  String filename = "/" + currentDate + ".csv";
  currentFile = SPIFFS.open(filename, FILE_APPEND);

  if (!currentFile) {
    Serial.println("Failed to open log file");
    return;
  }

  if (currentFile.size() == 0) {
    currentFile.println("Time,Status,Detected(cm)");
  }

  Serial.printf("Logging to: %s\n", filename.c_str());
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(256000);
  delay(1000);  // Give time for Serial Monitor
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalTimeout(120);
if(!wm.autoConnect("ECHO","12345678")) {
    Serial.println("failed to connect and hit timeout");
  }
  bootCount++;
  Serial.println("Boot Count: " + String(bootCount));

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  // Get time from NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for NTP time...");
    delay(1000);
  }

  // Update CSV file
  updateLogFile();

  // Setup Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // === Sensor Reading & Logging ===
  if (radar.readFrame()) {
    PresenceData data = radar.getData();

    // Get current time again
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to get time");
    } else {
      // Get date and time
      char dateStr[11], timeStr[9];
      strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

      String today = String(dateStr);
      if (today != currentDate) {
        updateLogFile();
      }

      String status;
      bool statusBool = false;
      if ((data.motion_energy != 0||data.static_energy!=0) && (data.motion_distance<180||data.static_distance<180)) {
        status = "Rented";
        statusBool = true;
      } else {
        status = "Empty";
        statusBool = false;
      }

      // Print to Serial
      Serial.printf("Time: %s | Status: %s | Detected: %dcm\n",
                    timeStr, status.c_str(), data.detect_distance);

      // Log to CSV
      if (currentFile) {
        currentFile.printf("%s,%s,%d\n", timeStr, status.c_str(), data.detect_distance);
        currentFile.flush();
      }

      // Upload to Firebase
      char firebasePath[64];
      snprintf(firebasePath, sizeof(firebasePath), "/logs/%s/%s", dateStr, timeStr);

      Firebase.setBool(fbdo, String(firebasePath) + "/status", statusBool);
      Firebase.setInt(fbdo, String(firebasePath) + "/distance", data.detect_distance);
    }
  }

  // === Go to Deep Sleep ===
  Serial.println("Going to deep sleep for 10 seconds...");
  esp_sleep_enable_timer_wakeup(10 * 1000000ULL);  // 58 seconds
  esp_deep_sleep_start();
}

void loop() {
  // loop() will not run in deep sleep mode
}
