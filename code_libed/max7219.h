
#ifndef MAX7219_H
#define MAX7219_H

#include <Arduino.h>
#include <SPI.h>

#define OP_NOOP        0x00
#define OP_DIGIT0      0x01
#define OP_DECODEMODE  0x09
#define OP_INTENSITY   0x0A
#define OP_SCANLIMIT   0x0B
#define OP_SHUTDOWN    0x0C
#define OP_DISPLAYTEST 0x0F

class MAX7219 {
  private:
    int mosi;
    int clk;
    int cs;

    byte status[8];

    void spiTransfer(volatile byte opcode, volatile byte data) {
      digitalWrite(cs, LOW);
      shiftOut(mosi, clk, MSBFIRST, opcode);
      shiftOut(mosi, clk, MSBFIRST, data);
      digitalWrite(cs, HIGH);
    }

  public:
    MAX7219(int mosi, int clk, int cs) {
      this->mosi = mosi;
      this->clk = clk;
      this->cs = cs;

      pinMode(mosi, OUTPUT);
      pinMode(clk, OUTPUT);
      pinMode(cs, OUTPUT);
      digitalWrite(cs, HIGH);

      spiTransfer(OP_DISPLAYTEST, 0); // exit test mode
      spiTransfer(OP_SCANLIMIT, 7); // scan limit all digits
      spiTransfer(OP_DECODEMODE, 0); // no decode all digits
      spiTransfer(OP_SHUTDOWN, 1); // normal operation

      spiTransfer(OP_INTENSITY, 1); // brightness 1->15

      // clear buffer
      for (int i = 0; i < 8; i++) {
        status[i] = 0x00;
        spiTransfer(OP_DIGIT0 + i, status[i]);
      }
    }

    void setLed(int row, int col, bool state) {
      if (row < 0 || row > 7 || col < 0 || col > 7) return; // bounds check

      byte mask = (0x80 >> col); 
      if (state) {
        status[row] |= mask;
      } else {
        status[row] &= ~mask;
      }
      // Write the updated byte back into that row’s register:
      spiTransfer(OP_DIGIT0 + row, status[row]);
    }
};

#endif
