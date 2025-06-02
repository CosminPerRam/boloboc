
#pragma once

#include <Wire.h>

#define REG_OPR_MODE    0x3D
#define REG_SYS_TRIGGER 0x3F
#define REG_EULER_H_LSB 0x1A

#define MODE_CONFIG     0x00
#define MODE_NDOF       0x0C

struct BNOAngles {
  float x;
  float y;
  float z;
};

class BNO055 {
public:
  BNO055(uint8_t addr = 0x28) : address(addr) {}

  bool begin() {
    Wire.begin();

    // config mode
    if (!write8(REG_OPR_MODE, MODE_CONFIG)) return false;

    // crystal
    if (!write8(REG_SYS_TRIGGER, 0x80)) return false;

    // NDOF fused-sensor mode
    if (!write8(REG_OPR_MODE, MODE_NDOF)) return false;

    return true;
  }

  bool getOrientation(BNOAngles &outAngles) {
    uint8_t buffer[6];
    if (!readLen(REG_EULER_H_LSB, buffer, 6)) {
      return false;
    }

    // combine LSB + MSB
    int16_t rawH = (int16_t)((buffer[1] << 8) | buffer[0]);
    int16_t rawR = (int16_t)((buffer[3] << 8) | buffer[2]);
    int16_t rawP = (int16_t)((buffer[5] << 8) | buffer[4]);

    // convert 1/16 units
    outAngles.x = (float)rawH / 16.0f;
    outAngles.y = (float)rawR / 16.0f;
    outAngles.z = (float)rawP / 16.0f;

    return true;
  }

private:
  uint8_t address;

  bool write8(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);

    Wire.write(reg);
    Wire.write(value);

    bool result = (Wire.endTransmission() == 0);
    delay(25);
    return result;
  }

  bool readLen(uint8_t startReg, uint8_t *buffer, uint8_t length) {
    Wire.beginTransmission(address);
    Wire.write(startReg);

    if (Wire.endTransmission(false) != 0) {
      return false;
    }

    Wire.requestFrom(address, length);

    for (uint8_t i = 0; i < length; i++) {
      if (Wire.available() < 1) return false;
      buffer[i] = Wire.read();
    }

    return true;
  }
};
