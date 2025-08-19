#pragma once

// library for controlling a 7-segment graphic display
// uses the graphics engine from GyverGFX

#include <SPI.h>
#include "GyverGFX.h"

// here you can set the SPI frequency
SPISettings MAX_SPI_SETT(6000000, MSBFIRST, SPI_MODE0);

template <byte CS, byte WW, byte HH>
struct MaxDisp : public GyverGFX {
  MaxDisp() : GyverGFX(WW * 4 * 4, HH * 2 * 6) {}

  void begin() {
    SPI.begin();
    pinMode(CS, OUTPUT);

    sendCMD(0xF, 0x00);  // disable test mode
    sendCMD(0x9, 0x00);  // disable decode mode
    sendCMD(0xA, 0x00);  // brightness minimum
    sendCMD(0xB, 0x07);  // show all digits
    sendCMD(0xC, 0x01);  // turn on
  }

  void setBright(uint8_t value) {
    sendCMD(0x0a, value);        // brightness 0-15
  }

  void beginData() {
    SPI.beginTransaction(MAX_SPI_SETT);
    digitalWrite(CS, 0);
  }

  void endData() {
    digitalWrite(CS, 1);
    SPI.endTransaction();
  }

  void dot(int x, int y, uint8_t fill = 1) {
    // choose indicator
    byte y6 = (y * 342) >> 11;     // ==(y/6); 342 == (1 << 11) / 6 + 1
    byte& b = getByte(x >> 2, y6); // x/4, y/6
    
    // x and y inside the indicator (sub-matrix 4x6)
    x = x & 3;    // x%4
    y -= y6 * 6;  // y%6
    
    // sum with shift, unique address of each pixel
    byte z = x | (y << 2);
    switch (z) {
      case 0x9:  bitWrite(b, 0, fill); break;
      case 0x4:  bitWrite(b, 1, fill); break;
      case 0xC:  bitWrite(b, 2, fill); break;
      case 0x11: bitWrite(b, 3, fill); break;
      case 0xE:  bitWrite(b, 4, fill); break;
      case 0x6:  bitWrite(b, 5, fill); break;
      case 0x1:  bitWrite(b, 6, fill); break;
      //case 0x13: bitWrite(b, 7, fill); break;
    }
  }

  void setByte(int x, int y, byte b) {
    getByte(x, y) = b;
  }
  byte& getByte(int x, int y) {
    //return buf[(y >> 1) * (WW * 8) + (x >> 2) * 8 + (x % 4) + (y % 2) * 4];
    return buf[(y >> 1) * (WW << 3) + ((x >> 2) << 3) + (x & 3) + ((y & 1) << 2)];
  }

  void update() {
    int N = WW * HH;
    for (int i = 0; i < 8; i++) {
        beginData();
        for (int j = 0; j < N; j++) {
            int group = j / 10;           // define group of 10 segments
            int indexInGroup = j % 10;    // position inside group
            int displayIndex;

            // even group → forward order, odd group → reverse order
            if (group % 2 == 0) {
                displayIndex = j;
            } else {
                displayIndex = group * 10 + (9 - indexInGroup);
            }

            // invert order to match original code
            displayIndex = N - 1 - displayIndex;

            sendData(i + 1, buf[(displayIndex << 3) + i]);
        }
        endData();
    }
  }

  void clear() {
    //fill(0);
    memset(buf, 0, WW * HH * 8);
  }

  void fill(byte b = 0) {
    /*for (int i = 0; i < 8; i++) {
      beginData();
      for (int j = 0; j < WW * HH; j++) sendData(i + 1, b);
      endData();
    }*/
    memset(buf, b, WW * HH * 8);
  }

  void sendByte(uint8_t address, uint8_t value) {
    beginData();
    sendData(address + 1, value);
    endData();
  }

  void sendCMD(uint8_t address, uint8_t value) {
    beginData();
    for (int i = 0; i < WW * HH; i++) sendData(address, value);
    endData();
  }

  void sendData(uint8_t address, uint8_t value) {
    SPI.transfer(address);
    SPI.transfer(value);
  }

  byte buf[WW * HH * 8];
};
