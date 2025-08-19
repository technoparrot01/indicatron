#include "MAX7219.h"

#define MAX_CS 5
#define DW 10
#define DH 5

MaxDisp<MAX_CS, DW, DH> disp;

#include <ThreeWire.h>
#include <RtcDS1302.h>

#define DS1302_IO    15
#define DS1302_SCLK   2
#define DS1302_CE    13

ThreeWire myWire(DS1302_IO, DS1302_SCLK, DS1302_CE);
RtcDS1302<ThreeWire> Rtc(myWire);

void ensureRtcRunningAndValid() {
  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
  if (!Rtc.IsDateTimeValid()) {
    RtcDateTime compiled(__DATE__, __TIME__);
    Rtc.SetDateTime(compiled);
  }
}

const uint8_t SEG_MAP[10] PROGMEM = {
  0b1111110,
  0b0110000,
  0b1101101,
  0b1111001,
  0b0110011,
  0b1011011,
  0b1011111,
  0b1110000,
  0b1111111,
  0b1111011
};

void segRect(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  for (int i = 0; i < h; i++) disp.line(x, y + i, x + w - 1, y + i);
}

void drawSegDigit(int x, int y, int w, int h, uint8_t digit, int t) {
  if (digit > 9) digit = 0;

  int pad = max(1, t);
  int ix = x + pad;
  int iy = y + pad;
  int iw = max(3 * t, w - 2 * pad);
  int ih = max(5 * t, h - 2 * pad);

  int A_w = iw - 2 * t;
  int A_h = t;
  int H_gap = t;

  int V_h = (ih - 3 * t) / 2;
  if (V_h < t) V_h = t;

  int leftX  = ix;
  int rightX = ix + iw - t;
  int midY   = iy + (ih - t) / 2;

  int Ax = ix + t, Ay = iy;
  int Gx = ix + t, Gy = midY;
  int Dx = ix + t, Dy = iy + ih - t;

  int Fx = leftX,  Fy = iy + t + H_gap;
  int Bx = rightX, By = iy + t + H_gap;
  int Ex = leftX,  Ey = midY + t + H_gap;
  int Cx = rightX, Cy = midY + t + H_gap;

  uint8_t mask = pgm_read_byte(&SEG_MAP[digit]);

  if (mask & 0b1000000) segRect(Ax, Ay, A_w, A_h);
  if (mask & 0b0100000) segRect(Bx, By, t, V_h);
  if (mask & 0b0010000) segRect(Cx, Cy, t, V_h);
  if (mask & 0b0001000) segRect(Dx, Dy, A_w, A_h);
  if (mask & 0b0000100) segRect(Ex, Ey, t, V_h);
  if (mask & 0b0000010) segRect(Fx, Fy, t, V_h);
  if (mask & 0b0000001) segRect(Gx, Gy, A_w, A_h);
}

void drawSegColon(int x, int y, int w, int h, int t, bool on) {
  if (!on) return;
  int dot = max(1, t);
  int cx = x + w / 2 - dot / 2;
  int topY = y + h / 3 - dot / 2;
  int botY = y + 2 * h / 3 - dot / 2;
  segRect(cx, topY, dot, dot);
  segRect(cx, botY, dot, dot);
}

void drawClockAuto(uint8_t hh, uint8_t mm, bool colonOn) {
  disp.clear();

  const int W = disp.W();
  const int H = disp.H();

  int marginX = max(1, W / 40);
  int marginY = max(1, H / 20);
  int colonW  = max(1, W / 24);
  int gap     = max(1, W / 80);

  int availableW = W - 2 * marginX - colonW - 5 * gap;
  int digitW = availableW / 4;
  int digitH = H - 2 * marginY;
  int t = max(1, min(digitW, digitH) / 8);

  int x0 = marginX;
  int y0 = marginY;

  uint8_t d0 = hh / 10;
  uint8_t d1 = hh % 10;
  uint8_t d2 = mm / 10;
  uint8_t d3 = mm % 10;

  drawSegDigit(x0, y0, digitW, digitH, d0, t);
  int x1 = x0 + digitW + gap;
  drawSegDigit(x1, y0, digitW, digitH, d1, t);
  int cx = x1 + digitW + gap;
  drawSegColon(cx, y0, colonW, digitH, t, colonOn);
  int x2 = cx + colonW + gap;
  drawSegDigit(x2, y0, digitW, digitH, d2, t);
  int x3 = x2 + digitW + gap;
  drawSegDigit(x3, y0, digitW, digitH, d3, t);

  disp.update();
}

void dvdBig() {
  const int W = max(12, disp.W() / 3);
  const int H = max(10, disp.H() / 3);

  static bool inited = false;
  static int x10, y10, vx, vy;

  if (!inited) {
    inited = true;
    x10 = ((disp.W() - W) / 2) * 10;
    y10 = ((disp.H() - H) / 2) * 10;
    vx = random(8, 14);
    vy = random(6, 12);
  }

  x10 += vx;
  y10 += vy;

  if (x10 <= 0) { x10 = 0; vx = -vx; }
  if (x10 >= (disp.W() - W) * 10) { x10 = (disp.W() - W) * 10; vx = -vx; }
  if (y10 <= 0) { y10 = 0; vy = -vy; }
  if (y10 >= (disp.H() - H) * 10) { y10 = (disp.H() - H) * 10; vy = -vy; }

  const int x = x10 / 10;
  const int y = y10 / 10;

  disp.clear();
  disp.line(x,         y,          x + W - 1, y);
  disp.line(x,         y,          x,         y + H - 1);
  disp.line(x + W - 1, y,          x + W - 1, y + H - 1);
  disp.line(x,         y + H - 1,  x + W - 1, y + H - 1);

  int s = 1;
  int maxS_w = (W - 6) / (3 * 5);
  int maxS_h = (H - 6) / 7;
  if (maxS_w > 1 && maxS_h > 1) s = min(maxS_w, maxS_h);

  disp.setScale(s);
  int textW = 3 * 5 * s;
  int textH = 7 * s;
  int tx = x + (W - textW) / 2;
  int ty = y + (H - textH) / 2;

  disp.setCursor(tx, ty);
  disp.print("DVD");

  disp.update();
  delay(10);
}

void clock_from_rtc() {
  static bool dots = false;
  static uint32_t lastBlink = 0;
  uint32_t nowMs = millis();
  if (nowMs - lastBlink >= 500) {
    lastBlink = nowMs;
    dots = !dots;
  }
  RtcDateTime now = Rtc.GetDateTime();
  uint8_t hh = now.Hour();
  uint8_t mm = now.Minute();
  drawClockAuto(hh, mm, dots);
}

void setup() {
  randomSeed(analogRead(0));
  disp.begin();
  disp.setBright(10);
  disp.textDisplayMode(GFX_ADD);
  ensureRtcRunningAndValid();
}

void loop() {
  clock_from_rtc();
  // dvdBig();
  delay(10);
}
