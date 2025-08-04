#include "LD2410B.h"

LD2410B radar(Serial2);

void setup() {
  Serial.begin(115200);
  radar.begin(256000);  // Default baud rate
}

void loop() {
  if (radar.readFrame()) {
    PresenceData data = radar.getData();
    Serial.print("Target: ");
    switch (data.target_state) {
      case 0x00: Serial.print("None"); break;
      case 0x01: Serial.print("Moving"); break;
      case 0x02: Serial.print("Stationary"); break;
      case 0x03: Serial.print("Both"); break;
    }
    Serial.print(" | Motion: ");
    Serial.print(data.motion_distance);
    Serial.print("cm, E=");
    Serial.print(data.motion_energy);
    Serial.print(" | Static: ");
    Serial.print(data.static_distance);
    Serial.print("cm, E=");
    Serial.print(data.static_energy);
    Serial.print(" | Detected: ");
    Serial.print(data.detect_distance);
    Serial.print("cm");

Serial.print(" | Status: ");
if ( data.motion_energy != 0 && data.target_state != 0x00) {
  Serial.print("The room is rented");
} else {
  Serial.println("Empty");
}


  }
}
