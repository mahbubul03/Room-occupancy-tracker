#include "LD2410B.h"

LD2410B::LD2410B(HardwareSerial& serial) : _serial(serial) {}

void LD2410B::begin(uint32_t baud) {
  _serial.begin(baud);
}

PresenceData LD2410B::getData() {
  return data;
}

bool LD2410B::readFrame() {
  if (_serial.available() < 20) return false;

  uint8_t buffer[64];
  size_t i = 0;
  unsigned long start = millis();

  // Sync to frame header F4 F3 F2 F1
  while (_serial.available() && millis() - start < 50) {
    if (_serial.peek() == 0xF4) {
      buffer[i++] = _serial.read();
      if (_serial.available() >= 3) {
        buffer[i++] = _serial.read(); // F3
        buffer[i++] = _serial.read(); // F2
        buffer[i++] = _serial.read(); // F1
        break;
      }
    } else {
      _serial.read();
    }
  }

  if (i < 4) return false;

  // Read 2-byte length (little endian)
  while (_serial.available() < 2);
  buffer[i++] = _serial.read(); // len L
  buffer[i++] = _serial.read(); // len H
  uint16_t dataLen = buffer[4] | (buffer[5] << 8);

  // Total length = 4 + 2 + dataLen + 4 (footer)
  uint16_t totalLen = 10 + dataLen;
  while (_serial.available() < (totalLen - i));

  for (; i < totalLen; ++i) {
    buffer[i] = _serial.read();
  }

  return parseFrame(buffer, totalLen);
}

bool LD2410B::parseFrame(uint8_t* buffer, size_t len) {
  if (len < 20) return false;

  if (buffer[6] != 0x02 || buffer[7] != 0xAA) return false; // Not basic info frame

  data.target_state    = buffer[8];
  data.motion_distance = buffer[9] | (buffer[10] << 8);
  data.motion_energy   = buffer[11];
  data.static_distance = buffer[12] | (buffer[13] << 8);
  data.static_energy   = buffer[14];
  data.detect_distance = buffer[15] | (buffer[16] << 8);

  return true;
}
