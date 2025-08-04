#include <WiFi.h>
#include <time.h>
#include "FS.h"
#include "SPIFFS.h"
#include "LD2410B.h"

const char* ssid     = "Mahbub";
const char* password = "mahbub99";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 6 * 3600;
const int   daylightOffset_sec = 0;


LD2410B radar(Serial2);
String currentDate = "";
File currentFile;

void setup() {
  Serial.begin(115200);
  Serial2.begin(256000);


  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");


  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }


  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for NTP time...");
    delay(1000);
  }

  updateLogFile();
}

void loop() {
  if (radar.readFrame()) {
    PresenceData data = radar.getData();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to get time");
      return;
    }

    // Check if date changed
    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    String today = String(dateStr);
    if (today != currentDate) {
      updateLogFile(); // new day → new file
    }

    // Create time string
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

    // Determine status
    String status;
    if (data.motion_energy != 0 && data.target_state != 0x00) {
      status = "Rented";
    } else {
      status = "Empty";
    }

    // Print to Serial
    Serial.printf("Time: %s | Status: %s | Detected: %dcm\n",
                  timeStr, status.c_str(), data.detect_distance);

    // Log to CSV
    if (currentFile) {
      currentFile.printf("%s,%s,%d\n", timeStr, status.c_str(), data.detect_distance);
      currentFile.flush();
    }
  }

  delay(2000);  // Delay between reads
}

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

  // Write header if new file
  if (currentFile.size() == 0) {
    currentFile.println("Time,Status,Detected(cm)");
  }

  Serial.printf("Logging to: %s\n", filename.c_str());
}
