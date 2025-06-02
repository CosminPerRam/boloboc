#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include "LedControl.h"

#define LOOP_DELAY 250
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

enum Axis {
  X = 0,
  Y,
  Z
};

Axis g_axis = X;
int g_snap = 0;
int g_point = 0;
bool g_displayNumerical = true;

bool g_btnSnapPressed = false;
bool g_btnAxisPressed = false;
bool g_btnTypePressed = false;

void setLed(int x, int y, bool state) {
  lc.setLed(0, x, y, state);
}

void clearLeds() {
  lc.clearDisplay(0);
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
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      setLed(x, y, letter[x][y]);
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
  Serial.println("");

  Serial.println("");
}

void loopDelay() {
  delay(LOOP_DELAY);
}

void updateDisplay() {
  lc.setLed(0, 0, 0, 1);
  lc.setLed(0, 3, 3, 1);
  lc.setLed(0, 7, 7, 1);
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
