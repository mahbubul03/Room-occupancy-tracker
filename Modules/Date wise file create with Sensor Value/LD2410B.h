#ifndef LD2410B_H
#define LD2410B_H

#include <Arduino.h>

struct PresenceData {
  uint8_t target_state;      // 0x00~0x03
  uint16_t motion_distance;  // cm
  uint8_t motion_energy;
  uint16_t static_distance;  // cm
  uint8_t static_energy;
  uint16_t detect_distance;  // cm
};

class LD2410B {
public:
  LD2410B(HardwareSerial& serial);
  void begin(uint32_t baud = 256000);
  bool readFrame();
  PresenceData getData();

private:
  HardwareSerial& _serial;
  PresenceData data;

  bool parseFrame(uint8_t* buffer, size_t len);
};

#endif
