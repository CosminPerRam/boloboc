#include <math.h>
#include "digits.h"
#include "max7219.h"
#include "bno055.h"

#define LOOP_DELAY 10
#define NUMERIC_CHANGE_DELAY 250
#define AXIS_CHANGE_DELAY 3000
#define SNAP_CHANGE_DELAY 3000
#define SETUP_DELAY 1000

#define SERIAL_BAUDRATE 9600

#define BTN_SNAP 3
#define BTN_AXIS 2
#define BTN_TYPE 0
#define LED_MOSI 10
#define LED_CLK 8
#define LED_CS 9

#define LED_BRIGHTNESS 1 // 1 - 15

#define NUMERIC_ROW_START 1
#define NUMERIC_COL_TENS_START 0
#define NUMERIC_COL_ONES_START 5
  
BNO055 bno = BNO055();
BNOAngles g_bnoAngles;

MAX7219 lc = MAX7219(LED_MOSI, LED_CLK, LED_CS);
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
  lc.setLed(x, y, state);
}

void resetLeds() {
  for(t_x = 0; t_x < 8; t_x++) {
    for(t_y = 0; t_y < 8; t_y++) {
      g_matrix[t_x][t_y] = false;
    }
  }
}

void renderLeds() {
  for(t_x = 0; t_x < 8; t_x++) {
    for(t_y = 0; t_y < 8; t_y++) {
      setLed(t_x, t_y, g_matrix[t_x][t_y]);
    }
  }
}

void checkTakeSnap() {
  if (!g_btnSnapPressed) {
    return;
  }

  g_snap = g_point;

  resetLeds();
  g_matrix[0][0] = true;
  g_matrix[7][0] = true;
  g_matrix[3][3] = true;
  g_matrix[4][4] = true;
  g_matrix[3][4] = true;
  g_matrix[4][3] = true;
  g_matrix[0][7] = true;
  g_matrix[7][7] = true;
  renderLeds();

  delay(SNAP_CHANGE_DELAY);
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

  resetLeds();
  for (t_x = 0; t_x < 3; t_x++) {
    for (t_y = 0; t_y < 3; t_y++) {
      g_matrix[t_x][t_y] = letter[t_x][t_y];
    }
  }
  renderLeds();

  delay(AXIS_CHANGE_DELAY);
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
  bno.getOrientation(g_bnoAngles);
  switch (g_axis) {
    case X:
      g_point = g_bnoAngles.x;
      break;
    case Y:
      g_point = g_bnoAngles.y;
      break;
    case Z:
      g_point = g_bnoAngles.z;
      break;
  }
  g_diff = g_snap - g_point;

  g_btnAxisPressed = digitalRead(BTN_AXIS);
  g_btnSnapPressed = digitalRead(BTN_SNAP);
  g_btnTypePressed = digitalRead(BTN_TYPE);
}

void serialPrintVars() {
  Serial.print("BNO: X: ");
  Serial.print(g_bnoAngles.x, 0);
  Serial.print("\tY: ");
  Serial.print(g_bnoAngles.y, 0);
  Serial.print("\tZ: ");
  Serial.print(g_bnoAngles.z, 0);
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

      bool on = false;
      if (start <= end) {
        if (theta >= start && theta <= end) {
          on = true;
        }
      } else {
        if (theta >= start || theta <= end) {
          on = true;
        }
      }
      g_matrix[i][j] = on;
    }
  }
}

static void drawDigit3x5(uint8_t digit, int leftCol) {
  if (digit > 9) return;

  for (int dr = 0; dr < 5; dr++) {
    uint8_t rowBits = DIGIT_FONT[digit][dr];
    for (int dc = 0; dc < 3; dc++) {
      bool pixelOn = (rowBits >> (2 - dc)) & 0x01;
      int r = NUMERIC_ROW_START + dr;
      int c = leftCol + dc;
      if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        g_matrix[r][c] = pixelOn ? 1 : 0;
      }
    }
  }
}

void computeNumericMatrix() {
  uint16_t number = abs(g_diff);
  if (number > 99) {
    resetLeds();
    renderLeds();
    return;
  }

  uint8_t tens = number / 10;
  uint8_t ones = number % 10;

  drawDigit3x5(tens, NUMERIC_COL_TENS_START);
  drawDigit3x5(ones, NUMERIC_COL_ONES_START);
}

void updateDisplay() {
  resetLeds();

  if (g_displayNumerical) {
    computeNumericMatrix();
  } else {
    computeRotatedMatrix();
  }

  renderLeds();
}

void setup(void) 
{
  pinMode(BTN_SNAP, INPUT_PULLDOWN);
  pinMode(BTN_AXIS, INPUT_PULLDOWN);
  pinMode(BTN_TYPE, INPUT_PULLDOWN);

  Serial.begin(SERIAL_BAUDRATE);
  
  if(!bno.begin())
  {
    Serial.print("BNO055 failure.");
    while(1);
  }

  lc.setBrightness(LED_BRIGHTNESS);
  
  delay(SETUP_DELAY);
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
