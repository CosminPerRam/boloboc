#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include "LedControl.h"
#include <math.h>
#include "digits.h"

#define LOOP_DELAY 0
#define NUMERIC_CHANGE_DELAY 250
#define AXIS_CHANGE_DELAY 3000
#define SNAP_CHANGE_DELAY 3000

#define BTN_SNAP 3
#define BTN_AXIS 2
#define BTN_TYPE 0
#define LED_DATA 10
#define LED_CLK 8
#define LED_CS 9
  
Adafruit_BNO055 bno = Adafruit_BNO055();
sensors_event_t g_bnoEvent;

LedControl lc = LedControl(LED_DATA, LED_CLK, LED_CS);
bool g_matrix[8][8] = { 0 };

enum Axis {
  X = 0,
  Y,
  Z
};

Axis g_axis = X;
int g_snap = 0;
int g_point = 0;
int g_diff = 0;
bool g_displayNumerical = true;

bool g_btnSnapPressed = false;
bool g_btnAxisPressed = false;
bool g_btnTypePressed = false;

int t_x, t_y;

void setLed(int x, int y, bool state) {
  lc.setLed(0, x, y, state);
}

void clearLeds() {
  for(t_x = 0; t_x < 8; t_x++) {
    for(t_y = 0; t_y < 8; t_y++) {
      setLed(t_x, t_y, false);
    }
  }
}

void resetLeds() {
  for(t_x = 0; t_x < 8; t_x++) {
    for(t_y = 0; t_y < 8; t_y++) {
      g_matrix[t_x][t_y] = false;
    }
  }
}

void checkTakeSnap() {
  if (!g_btnSnapPressed) {
    return;
  }

  g_snap = g_point;

  clearLeds();
  setLed(0, 0, true);
  setLed(7, 0, true);
  setLed(3, 3, true);
  setLed(4, 4, true);
  setLed(3, 4, true);
  setLed(4, 3, true);
  setLed(0, 7, true);
  setLed(7, 7, true);
  delay(SNAP_CHANGE_DELAY);
  clearLeds();
}

void printAxisChange() {
  bool letter[3][3] = { false };
  switch (g_axis) {
    case X:
      letter[0][0] = true;
      letter[0][2] = true;
      letter[1][1] = true;
      letter[2][0] = true;
      letter[2][2] = true;
      break;
    case Y:
      letter[0][0] = true;
      letter[0][2] = true;
      letter[1][1] = true;
      letter[2][1] = true;
      break;
    case Z:
      letter[0][0] = true;
      letter[0][1] = true;
      letter[0][2] = true;
      letter[1][1] = true;
      letter[2][0] = true;
      letter[2][1] = true;
      letter[2][2] = true;
      break;
  }

  clearLeds();
  for (t_x = 0; t_x < 3; t_x++) {
    for (t_y = 0; t_y < 3; t_y++) {
      setLed(t_x, t_y, letter[t_x][t_y]);
    }
  }

  delay(AXIS_CHANGE_DELAY);
  clearLeds();
}

void checkCycleAxis() {
  if (!g_btnAxisPressed) {
    return;
  }

  g_snap = 0;
  g_point = 0;
  g_diff = 0;
  switch (g_axis) {
    case X:
      g_axis = Y;
      break;
    case Y:
      g_axis = Z;
      break;
    case Z:
      g_axis = X;
      break;
  }

  printAxisChange();
}

void checkToggleNumericalDisplay() {
  if (!g_btnTypePressed) {
    return;
  }

  g_displayNumerical = !g_displayNumerical;
  delay(NUMERIC_CHANGE_DELAY);
}

void collectData() {
  bno.getEvent(&g_bnoEvent);
  switch (g_axis) {
    case X:
      g_point = g_bnoEvent.orientation.x;
      break;
    case Y:
      g_point = g_bnoEvent.orientation.y;
      break;
    case Z:
      g_point = g_bnoEvent.orientation.z;
      break;
  }
  g_diff = g_snap - g_point;

  g_btnAxisPressed = digitalRead(BTN_AXIS);
  g_btnSnapPressed = digitalRead(BTN_SNAP);
  g_btnTypePressed = digitalRead(BTN_TYPE);
}

void serialPrintVars() {
  Serial.print("BNO: X: ");
  Serial.print(g_bnoEvent.orientation.x, 0);
  Serial.print("\tY: ");
  Serial.print(g_bnoEvent.orientation.y, 0);
  Serial.print("\tZ: ");
  Serial.print(g_bnoEvent.orientation.z, 0);
  Serial.println("");

  Serial.print(" IO: AXIS: ");
  Serial.print(g_btnAxisPressed);
  Serial.print("\tSNAP: ");
  Serial.print(g_btnSnapPressed);
  Serial.print("\tTYPE: ");
  Serial.print(g_btnTypePressed);
  Serial.println("");

  Serial.print("   : Axis: ");
  Serial.print(g_axis);
  Serial.print("\tSnap: ");
  Serial.print(g_snap);
  Serial.print("\tType: ");
  Serial.print(g_displayNumerical);
  Serial.print("\tPoint: ");
  Serial.print(g_point);
  Serial.print("\tDiff: ");
  Serial.print(g_diff);
  Serial.println("");

  Serial.println("");
}

void loopDelay() {
  delay(LOOP_DELAY);
}

void computeRotatedMatrix() {
    float start = g_diff - 90.0f;
    float end   = g_diff + 90.0f;

    while (start < 0.0f)    start += 360.0f;
    while (start >= 360.0f) start -= 360.0f;
    while (end < 0.0f)      end   += 360.0f;
    while (end >= 360.0f)   end   -= 360.0f;

    int i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {

            float x = (float)j - 3.5f;
            float y = 3.5f - (float)i;

            float theta = atan2f(y, x) * (180.0f / PI);
            if (theta < 0.0f) {
                theta += 360.0f;
            }

            uint8_t on = 0;
            if (start <= end) {
                if (theta >= start && theta <= end) {
                    on = 1;
                }
            } else {
                if (theta >= start || theta <= end) {
                    on = 1;
                }
            }
            g_matrix[i][j] = on;
        }
    }
}

static void drawDigit3x5(uint8_t digit, int topRow, int leftCol) {
  if (digit > 9) return;
  for (int dr = 0; dr < 5; dr++) {
    uint8_t rowBits = DIGIT_FONT[digit][dr];
    for (int dc = 0; dc < 3; dc++) {
      bool pixelOn = (rowBits >> (2 - dc)) & 0x01;
      int r = topRow + dr;
      int c = leftCol + dc;
      if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        g_matrix[r][c] = pixelOn ? 1 : 0;
      }
    }
  }
}

bool writeTwoDigitNumber(uint16_t number) {
  if (number > 99) return false;

  uint8_t tens  = number / 10;
  uint8_t ones  = number % 10;

  const int topRow = 1;
  const int leftColTens = 0;
  const int leftColOnes = 5;

  drawDigit3x5(tens, topRow, leftColTens);
  drawDigit3x5(ones, topRow, leftColOnes);

  return true;
}

void computeNumericMatrix() {
  bool written = writeTwoDigitNumber(abs(g_diff));
  if (!written) {
    clearLeds();
  }
}

void updateDisplay() {
  resetLeds();
  if (g_displayNumerical) {
    computeNumericMatrix();
  } else {
    computeRotatedMatrix();
  }

  for(t_x = 0; t_x < 8; t_x++) {
    for(t_y = 0; t_y < 8; t_y++) {
      setLed(t_x, t_y, g_matrix[t_x][t_y]);
    }
  }
}

void setup(void) 
{
  pinMode(BTN_SNAP, INPUT_PULLDOWN);
  pinMode(BTN_AXIS, INPUT_PULLDOWN);
  pinMode(BTN_TYPE, INPUT_PULLDOWN);

  Serial.begin(9600);
  
  if(!bno.begin())
  {
    Serial.print("BNO055 failure.");
    while(1);
  }
  
  delay(1000);

  bno.setExtCrystalUse(true);

  lc.shutdown(0,false);
  lc.setIntensity(0,1);
  lc.clearDisplay(0);
}

void loop(void) 
{
  collectData();

  checkTakeSnap();
  checkCycleAxis();
  checkToggleNumericalDisplay();

  serialPrintVars();

  updateDisplay();
  
  loopDelay();
}
