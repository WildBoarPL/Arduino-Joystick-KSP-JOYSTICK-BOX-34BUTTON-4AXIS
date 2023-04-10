//KSP-CONTROLL-BOX
//USE w ProMicro
//Tested in WIN10 + KSP 1 (with Fly-By-Wire MOD) + AntiMicroX
//ARDUINO IDE
//2.0.4

#include <Keypad.h>
#include <Joystick.h>

#define ENABLE_PULLUPS
#define NUMROTARIES 1
#define NUMBUTTONS 36
#define NUMROWS 6
#define NUMCOLS 6


byte buttons[NUMROWS][NUMCOLS] = {
  { 0, 1, 2, 3, 4, 5 },
  { 6, 7, 8, 9, 10, 11 },
  { 12, 13, 14, 15, 16, 17 },
  { 18, 19, 20, 21, 22, 23 },
  { 24, 25, 26, 27, 28, 29 },
  { 30, 31, 32, 33, 34, 35 }  // 32,33,34,35 - reserved fopr HAT switch
};

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar;
  int cwchar;
  volatile unsigned char state;
};

rotariesdef rotaries[NUMROTARIES]{
  { 0, 1, 32, 33, 0 }
};

#define DIR_CCW 0x10
#define DIR_CW 0x20
#define R_START 0x0

#ifdef HALF_STEP
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  { R_START_M, R_CW_BEGIN, R_CCW_BEGIN, R_START },
  // R_CCW_BEGIN
  { R_START_M | DIR_CCW, R_START, R_CCW_BEGIN, R_START },
  // R_CW_BEGIN
  { R_START_M | DIR_CW, R_CW_BEGIN, R_START, R_START },
  // R_START_M (11)
  { R_START_M, R_CCW_BEGIN_M, R_CW_BEGIN_M, R_START },
  // R_CW_BEGIN_M
  { R_START_M, R_START_M, R_CW_BEGIN_M, R_START | DIR_CW },
  // R_CCW_BEGIN_M
  { R_START_M, R_CCW_BEGIN_M, R_START_M, R_START | DIR_CCW },
};
#else
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  { R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START },
  // R_CW_FINAL
  { R_CW_NEXT, R_START, R_CW_FINAL, R_START | DIR_CW },
  // R_CW_BEGIN
  { R_CW_NEXT, R_CW_BEGIN, R_START, R_START },
  // R_CW_NEXT
  { R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START },
  // R_CCW_BEGIN
  { R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START },
  // R_CCW_FINAL
  { R_CCW_NEXT, R_CCW_FINAL, R_START, R_START | DIR_CCW },
  // R_CCW_NEXT
  { R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START },
};
#endif

byte rowPins[NUMROWS] = { 15, 14, 16, 10, 9, 8 };
byte colPins[NUMCOLS] = { 7, 6, 5, 4, 3, 2 };

#define joyY A0
#define joyX A1
#define joyT A2
#define joyR A3


int lastXAxisValue = -1;
int lastYAxisValue = -1;
int lastRAxisValue = -1;
int lastTAxisValue = -1;

// HAT button SETUP
const int hatU = 34;
const int hatL = 35;
const int hatD = 33;
const int hatR = 32;

// HAT STATE BUFOR
bool hatUState = false;
bool hatLState = false;
bool hatDState = false;
bool hatRState = false;

Keypad buttbx = Keypad(makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS);

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                   34, 1,                 // Button Count (0-31 for buttons, 32,33 for encoder ), Hat Switch Count
                   true, true, false,     // X and Y, but no Z Axis
                   false, false, false,   // No Rx, Ry, or Rz
                   true, true,            // YES rudder or throttle
                   false, false, false);  // No accelerator, brake, or steering


void setup() {
  Joystick.begin();

// ADJUST RANGE IF CABLES ARE CONNECTED DIFRENTLY 
  Joystick.setXAxisRange(0, 1023);
  Joystick.setYAxisRange(0, 1023);
  Joystick.setRudderRange(0, 1023);
  Joystick.setThrottleRange(0, 1023);
  
  rotary_init();
}

void loop() {
  // DO THE JOYSTICK JOB
  CheckAllEncoders();

  CheckAllButtons();

  CheckAllAxes();

  Joystick.sendState();
}

void CheckAllAxes(void) {

  // Print the full X/Y joystick values (0-1023)
  //Serial.print("\tX="),Serial.print(analogRead(joyT));
  //Serial.println();

  const int currentTAxisValue = analogRead(joyT);
  if (currentTAxisValue != lastTAxisValue) {
    Joystick.setThrottle(currentTAxisValue);
    lastTAxisValue = currentTAxisValue;
  }

  const int currentRAxisValue = analogRead(joyR);
  if (currentRAxisValue != lastRAxisValue) {
    Joystick.setRudder(currentRAxisValue);
    lastRAxisValue = currentRAxisValue;
  }

  const int currentXAxisValue = analogRead(joyX);
  if (currentXAxisValue != lastXAxisValue) {
    Joystick.setXAxis(currentXAxisValue);
    lastXAxisValue = currentXAxisValue;
  }

  const int currentYAxisValue = analogRead(joyY);
  if (currentYAxisValue != lastYAxisValue) {
    Joystick.setYAxis(currentYAxisValue);
    lastYAxisValue = currentYAxisValue;
  }
}

int kInt = -1;

void CheckAllButtons(void) {
  if (buttbx.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (buttbx.key[i].stateChanged) {
        switch (buttbx.key[i].kstate) {
          case PRESSED:
          case HOLD:
            kInt = buttbx.key[i].kchar;
            if (kInt == hatU || kInt == hatR || kInt == hatL || kInt == hatD) {
              switch (kInt) {
                case hatU:
                  hatUState = true;
                  break;
                case hatR:
                  hatRState = true;
                  break;
                case hatD:
                  hatDState = true;
                  break;
                case hatL:
                  hatLState = true;
                  break;
              }
            } else {
              Joystick.setHatSwitch(0, -1);
              Joystick.setButton(buttbx.key[i].kchar, 1);
            }
            break;
          case RELEASED:
          case IDLE:
            kInt = buttbx.key[i].kchar;
            if (kInt == hatU || kInt == hatR || kInt == hatL || kInt == hatD) {
              switch (kInt) {
                case hatU:
                  hatUState = false;
                  break;
                case hatR:
                  hatRState = false;
                  break;
                case hatD:
                  hatDState = false;
                  break;
                case hatL:
                  hatLState = false;
                  break;
              }
            } else {
              Joystick.setButton(buttbx.key[i].kchar, 0);
            }
            break;
        }
      }
    }
    UpdateHat();
  }
}
void UpdateHat() {

  int direction = -1;

  if (hatUState && !hatLState && !hatRState) {
    direction = 0;
  } else if (hatUState && !hatLState && hatRState) {
    direction = 45;
  } else if (!hatDState && !hatUState && hatRState) {
    direction = 90;
  } else if (hatRState && hatDState && !hatUState) {
    direction = 135;
  } else if (hatDState && !hatLState && !hatRState) {
    direction = 180;
  } else if (hatDState && !hatLState && !hatRState) {
    direction = 180;
  } else if (hatDState && hatLState && !hatRState) {
    direction = 225;
  } else if (hatLState && !hatDState && !hatUState) {
    direction = 270;
  } else if (hatLState && hatUState && !hatRState) {
    direction = 315;
  }

  //Serial.println(direction);

  Joystick.setHatSwitch(0, direction);
}

void rotary_init() {
  for (int i = 0; i < NUMROTARIES; i++) {
    pinMode(rotaries[i].pin1, INPUT);
    pinMode(rotaries[i].pin2, INPUT);
#ifdef ENABLE_PULLUPS
    digitalWrite(rotaries[i].pin1, HIGH);
    digitalWrite(rotaries[i].pin2, HIGH);
#endif
  }
}


unsigned char rotary_process(int _i) {
  unsigned char pinstate = (digitalRead(rotaries[_i].pin2) << 1) | digitalRead(rotaries[_i].pin1);
  rotaries[_i].state = ttable[rotaries[_i].state & 0xf][pinstate];
  return (rotaries[_i].state & 0x30);
}

void CheckAllEncoders(void) {
  for (int i = 0; i < NUMROTARIES; i++) {
    unsigned char result = rotary_process(i);
    if (result == DIR_CCW) {
      Joystick.setButton(rotaries[i].ccwchar, 1);
      delay(50);
      Joystick.setButton(rotaries[i].ccwchar, 0);
    };
    if (result == DIR_CW) {
      Joystick.setButton(rotaries[i].cwchar, 1);
      delay(50);
      Joystick.setButton(rotaries[i].cwchar, 0);
    };
  }
}