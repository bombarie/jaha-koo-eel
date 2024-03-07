#include <Arduino.h>
#include <EelMotor.h>
#include <Wire.h>
#include "sbus.h"
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// NeoPixel
#define PIN D9
#define NUMPIXELS 3

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

HardwareSerial MySerial0(0);

/* SBUS object, reading SBUS */
bfs::SbusRx sbus_rx(&MySerial0, D7, D6, true, false);

/* SBUS data */
bfs::SbusData data;

#define motor1a_pin D5
#define motor1b_pin D4
#define motor2a_pin D3
#define motor2b_pin D2

#define motor1_chan 4 // 6 Channels (ESP32-C3) (0-5) are availible
#define motor2_chan 5 // 6 Channels (ESP32-C3) (0-5) are availible

// setting PWM properties
const uint32_t freq = 2500;
const uint8_t resolution = 8;

EelMotor motor1(motor1a_pin, motor1b_pin, motor1_chan, resolution, freq);
EelMotor motor2(motor2a_pin, motor2b_pin, motor2_chan, resolution, freq);

u_long sbusPrevPacketTime;
bool sbusLost = false;

#define SBUS_VAL_MIN 191
#define SBUS_VAL_MAX 1793
#define SBUS_VAL_CENTER 992
#define SBUS_VAL_DEADBAND 6
#define SBUS_LOST_TIMEOUT 100
#define SBUS_SWITCH_MIN 192
#define SBUS_SWITCH_MAX 1792
#define SBUS_SWITCH_MIN_THRESHOLD 1400
#define SBUS_SWITCH_MAX_THRESHOLD 550

#define TX_ROLL 0
#define TX_PITCH 1
#define TX_THROTTLE 2
#define TX_YAW 3
#define TX_AUX1 4
#define TX_AUX2 5
#define TX_AUX3 6
#define TX_AUX4 7

enum HEAD_STATE
{
  STATE_1,
  STATE_2,
  STATE_3
};
HEAD_STATE headState = STATE_1;

enum BODY_STATE
{
  NOOD1,
  NOOD2,
  BOTH_NOODS
};
BODY_STATE bodyState = BOTH_NOODS;

// The CONNECTION_ESTABLISHED and CONNECTION_LOST states are kind of vanity states, in case I want to
// implement some kind of unique visual feedback for when the connection is lost or established.
enum CONNECTION_STATE
{
  DISCONNECTED,
  CONNECTION_ESTABLISHED,
  CONNECTION_LOST,
  CONNECTED
};
CONNECTION_STATE connectionState = DISCONNECTED;

#define SBUS_PACKET_PRINT_INTERVAL 100 // ms
u_long sbusPacketPrintPrevTime = 0;

float sinCounter = 0.0;
float sinCounterIncrement = 0.05;

bool doSineMovement = false;

byte hVal = 0;
byte vVal = 0;

int16_t motor1Val;
int16_t motor2Val;

int16_t n00d1a, n00d1b, n00d2a, n00d2b;
uint16_t throttle, throttleAdjusted;

int16_t noodVals[] = {0, 0, 0, 0};
int16_t noodAvgVals[] = {0, 0, 0, 0};
uint16_t n00dSegmentIdentifiers[] = {512, 640, 768, 896};

#define n00d_1a_Pin D8
#define n00d_1b_Pin D1
#define n00d_2a_Pin D10
#define n00d_2b_Pin D0

static const uint8_t nood1a_chan = 0;
static const uint8_t nood1b_chan = 1;
static const uint8_t nood2a_chan = 2;
static const uint8_t nood2b_chan = 3;

void initNoods();
void initPixels();
void updateBodyValues();
void updateBodyLighting();
void updateSerialIO();
void checkIncomingSerial();
void setn00d(uint8_t chan, uint8_t val);

void resetSbusData();
void setRGBLights();
void setLights_disconnected();

// define methods
void driveMotors();
void parseSBUS(bool serialPrint);
void updateHeadBodyState();
void updateHeadLighting();
void updateBodyLighting();
void calcMotorValues();

void setup()
{
  Serial.begin(115200);

  /* Begin the SBUS communication */
  sbus_rx.Begin();
  // sbus_tx.Begin();

  initNoods();

  motor1.reversed = true;
  motor2.reversed = true;

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;

  initPixels();

  Serial.println("Hello World!");
}

void loop()
{
  // pixels.clear(); // Set all pixel colors to 'off'

  // read SBUS
  parseSBUS(false);

  // determine how to set the head and body lights
  switch (connectionState)
  {
  case DISCONNECTED:
    resetSbusData();
    setLights_disconnected();
    break;
  case CONNECTION_ESTABLISHED:
    if (sbusLost)
    {
      connectionState = CONNECTION_LOST;
    }
    else
    {
      connectionState = CONNECTED;
    }
    break;
  case CONNECTION_LOST:
    if (!sbusLost)
    {
      connectionState = CONNECTION_ESTABLISHED;
    }
    break;
  case CONNECTED:
    if (sbusLost)
    {
      connectionState = CONNECTION_LOST;
    }
    break;
  }

  updateHeadBodyState();

  updateBodyValues();
  updateBodyLighting();

  updateHeadLighting();
  // updateBodyLighting();

  calcMotorValues();
  driveMotors();

  // delay a little.
  delay(1000 / 200);
}

void initNoods()
{
  pinMode(n00d_1a_Pin, OUTPUT);
  pinMode(n00d_1b_Pin, OUTPUT);
  pinMode(n00d_2a_Pin, OUTPUT);
  pinMode(n00d_2b_Pin, OUTPUT);

  // nood 1a
  ledcSetup(nood1a_chan, freq, resolution);
  ledcAttachPin(n00d_1a_Pin, nood1a_chan);

  // nood 1b
  ledcSetup(nood1b_chan, freq, resolution);
  ledcAttachPin(n00d_1b_Pin, nood1b_chan);

  // nood 2a
  ledcSetup(nood2a_chan, freq, resolution);
  ledcAttachPin(n00d_2a_Pin, nood2a_chan);

  // nood 2b
  ledcSetup(nood2b_chan, freq, resolution);
  ledcAttachPin(n00d_2b_Pin, nood2b_chan);

  setn00d(nood1a_chan, 0);
  setn00d(nood1b_chan, 0);
  setn00d(nood2a_chan, 0);
  setn00d(nood2b_chan, 0);
}

void initPixels()
{
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  delay(10);
  pixels.clear();
  pixels.show();
}

void updateBodyValues()
{
  // map throttle range to 0-1023
  throttle = map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1023);

  throttleAdjusted = 0; // used to store the adjusted throttle value

  // if (throttle & (0x1 << 9)) // 512
  // if (throttle & n00dSegmentIdentifiers[0] == n00dSegmentIdentifiers[0]) // 512
  if (throttle >> 7 == 0x4) // 0b100
  {
    // throttleAdjusted = throttle - (throttle >> 5);
    throttleAdjusted = throttle - 9;
    n00d1a = (throttle & 0x7E) - 9;
    n00d1a = map(constrain(n00d1a, 0, 55), 0, 55, 0, 255);
  }
  // if (throttle & (0x1 << 8)) // 256
  // if (throttle & n00dSegmentIdentifiers[1] == n00dSegmentIdentifiers[1]) // 640
  if (throttle >> 7 == 0x5) // 0b101
  {
    // throttle -= throttle - (throttle >> 5);
    throttleAdjusted = throttle - 10;
    n00d1b = (throttle & 0x7E) - 10;
    n00d1b = map(constrain(n00d1b, 0, 55), 0, 55, 0, 255);
  }
  // if (throttle & (0x1 << 7)) // 128
  // if (throttle & n00dSegmentIdentifiers[2] == n00dSegmentIdentifiers[2]) // 768
  if (throttle >> 7 == 0x6) // 0b110
  {
    throttleAdjusted = throttle - 9;
    n00d2a = (throttle & 0x7E) - 9;
    n00d2a = map(constrain(n00d2a, 0, 55), 0, 55, 0, 255);
  }
  // if (throttle & (0x1 << 6)) // 64
  // if (throttle & n00dSegmentIdentifiers[3] == n00dSegmentIdentifiers[3]) // 896
  if (throttle >> 7 == 0x7) // 0b111
  {
    throttleAdjusted = throttle - 9;
    n00d2b = (throttle & 0x7E) - 9;
    n00d2b = map(constrain(n00d2b, 0, 55), 0, 55, 0, 255);
  }

  noodVals[0] = n00d1a;
  noodVals[1] = n00d1b;
  noodVals[2] = n00d2a;
  noodVals[3] = n00d2b;

  noodAvgVals[0] = 0.85 * noodAvgVals[0] + 0.15 * noodVals[0];
  noodAvgVals[1] = 0.85 * noodAvgVals[1] + 0.15 * noodVals[1];
  noodAvgVals[2] = 0.85 * noodAvgVals[2] + 0.15 * noodVals[2];
  noodAvgVals[3] = 0.85 * noodAvgVals[3] + 0.15 * noodVals[3];
}

void updateBodyLighting()
{
  setn00d(nood1a_chan, noodAvgVals[0]);
  setn00d(nood1b_chan, noodAvgVals[1]);
  setn00d(nood2a_chan, noodAvgVals[2]);
  setn00d(nood2b_chan, noodAvgVals[3]);
}

uint8_t channelToPrint = 255;
void updateSerialIO()
{
  checkIncomingSerial();

  EVERY_N_MILLIS(100)
  {
    if (channelToPrint == 0) // only nood1a
    {
      Serial.print("n00d1a: ");
      Serial.print(n00d1a);
      Serial.print("\t in binary: ");
      Serial.println(n00d1a, BIN);
    }
    if (channelToPrint == 1) // only nood1b
    {
      Serial.print("n00d1b: ");
      Serial.print(n00d1b);
      Serial.print("\t in binary: ");
      Serial.println(n00d1b, BIN);
    }
    if (channelToPrint == 2) // only nood2a
    {
      if (throttle & (0x1 << 7)) // 128
      {
        Serial.print("TX_THROTTLE: ");
        Serial.print(data.ch[TX_THROTTLE]);
        Serial.print("\t throttle: ");
        Serial.print(throttle);
        Serial.print("\t (in binary: ");
        Serial.print(throttle, BIN);
      }
      Serial.print("\t n00d2a: ");
      Serial.print(n00d2a);
      Serial.print("\t in binary: ");
      Serial.println(n00d2a, BIN);
    }
    if (channelToPrint == 3) // only nood2b
    {
      if (throttle & (0x1 << 6)) // 64
      {
        Serial.print("TX_THROTTLE: ");
        Serial.print(data.ch[TX_THROTTLE]);
        Serial.print("\t throttle: ");
        Serial.print(throttle);
        Serial.print("\t (in binary: ");
        Serial.print(throttle, BIN);
      }
      Serial.print("n00d2b: ");
      Serial.print(n00d2b);
      Serial.print("\t in binary: ");
      Serial.println(n00d2b, BIN);
    }
    if (channelToPrint == 100) // everything
    {
      Serial.print("TX_THROTTLE: ");
      Serial.print(data.ch[TX_THROTTLE]);
      Serial.print("\t throttle: ");
      Serial.print(throttle);
      Serial.print("\t (in binary: ");
      Serial.println(throttle, BIN);
      Serial.print("n00d1a: ");
      Serial.print(n00d1a);
      Serial.print("\t n00d1b: ");
      Serial.print(n00d1b);
      Serial.print("\t n00d2a: ");
      Serial.print(n00d2a);
      Serial.print("\t n00d2b: ");
      Serial.println(n00d2b);
      Serial.println("");
    }
  }
}

void checkIncomingSerial()
{
  if (Serial.available() > 0)
  {
    char inChar = Serial.read();
    switch (inChar)
    {
    case '1':
      channelToPrint = 0;
      break;
    case '2':
      channelToPrint = 1;
      break;
    case '3':
      channelToPrint = 2;
      break;
    case '4':
      channelToPrint = 3;
      break;
    case 'a':
      channelToPrint = 100;
      break;
    case 'x':
      channelToPrint = 255;
      break;
    }

    while (Serial.available() > 0)
    {
      Serial.read();
    }
  }
}

void setn00d(uint8_t chan, uint8_t val)
{
  ledcWrite(chan, (255 - val));
}

void parseSBUS(bool serialPrint)
{
  if (sbus_rx.Read())
  {
    sbusPrevPacketTime = millis();
    if (connectionState == CONNECTION_LOST || connectionState == DISCONNECTED)
    {
      Serial.println("Regained SBUS connection");
      connectionState = CONNECTED;
    }

    /* Grab the received data */
    data = sbus_rx.data();

    /* Display the received data */
    if (millis() - sbusPacketPrintPrevTime > SBUS_PACKET_PRINT_INTERVAL)
    {
      //*
      if (Serial && serialPrint)
      {
        for (int8_t i = 0; i < data.NUM_CH; i++)
        {
          Serial.print(data.ch[i]);
          Serial.print("\t");
        }
        Serial.println();
      }
      //*/

      sbusPacketPrintPrevTime = millis();
    }
  }

  // if SBUS lost, reset the channels
  if (millis() - sbusPrevPacketTime > SBUS_LOST_TIMEOUT)
  {
    if (connectionState != DISCONNECTED)
    {
      Serial.print("Lost SBUS connection >> setting throttle/pitch/roll to ");
      Serial.print((SBUS_VAL_MIN + SBUS_VAL_MAX) / 2);
      Serial.println(" and yaw to 0 ");

      connectionState = DISCONNECTED;
    }
  }

  // if (connectionState == CONNECTION_LOST)
  // {
  //   resetSbusData();
  // }
}

void resetSbusData()
{
  for (int8_t i = 0; i < data.NUM_CH; i++)
  {
    data.ch[i] = SBUS_VAL_MIN;
    if (i < 4)
    {
      data.ch[i] = (SBUS_VAL_MIN + SBUS_VAL_MAX) / 2;
      if (i == TX_YAW)
        data.ch[i] = SBUS_VAL_MIN;
    }
  }
}

void updateHeadBodyState()
{
  // if (data.ch[TX_AUX2] < SBUS_SWITCH_MIN_THRESHOLD)
  // {
  //   headState = STATE_1;
  // }
  // else
  // {
  //   headState = STATE_2;
  // }

  if (data.ch[TX_AUX2] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    headState = STATE_1;
  }
  else if (data.ch[TX_AUX2] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    headState = STATE_2;
  }
  else
  {
    headState = STATE_3;
  }
}

// led 1 and 3 = eyes
// led 2 = mouth
void updateHeadLighting()
{
  uint8_t audioVal = constrain(map(data.ch[TX_YAW], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 255), 0, 255);
  switch (headState)
  {
  case STATE_1: // tmp: off
    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // left eye
    pixels.setPixelColor(1, pixels.Color(0, 0, 0)); // mouth
    pixels.setPixelColor(2, pixels.Color(0, 0, 0)); // right eye
    // pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // left eye
    // pixels.setPixelColor(1, pixels.Color(255, audioVal, audioVal)); // mouth
    // pixels.setPixelColor(2, pixels.Color(audioVal, 255, audioVal)); // right eye
    break;
  case STATE_2: // tmp: green
    pixels.setPixelColor(0, pixels.Color(0, audioVal, 0)); // left eye
    pixels.setPixelColor(1, pixels.Color(0, audioVal, 0));      // mouth
    pixels.setPixelColor(2, pixels.Color(0, audioVal, 0)); // right eye
    // pixels.setPixelColor(0, pixels.Color(audioVal, audioVal, audioVal)); // left eye
    // pixels.setPixelColor(1, pixels.Color(255, audioVal, audioVal));      // mouth
    // pixels.setPixelColor(2, pixels.Color(audioVal, audioVal, audioVal)); // right eye
    break;
  case STATE_3: // tmp: white
    pixels.setPixelColor(0, pixels.Color(audioVal, audioVal, audioVal)); // left eye
    pixels.setPixelColor(1, pixels.Color(audioVal, audioVal, audioVal));      // mouth
    pixels.setPixelColor(2, pixels.Color(audioVal, audioVal, audioVal)); // right eye
    // pixels.setPixelColor(0, pixels.Color(audioVal, audioVal, audioVal)); // left eye
    // pixels.setPixelColor(1, pixels.Color(255, audioVal, audioVal));      // mouth
    // pixels.setPixelColor(2, pixels.Color(audioVal, audioVal, audioVal)); // right eye
    break;
  }
  pixels.show(); // Send the updated pixel colors to the hardware.
}

void setRGBLights()
{
  // ??
}

void setLights_disconnected()
{
  // ??
}

void calcMotorValues()
{
  byte motorPowerRange = 255;

  /* position of AUX4 selects diff motor power ranges
  if (data.ch[TX_AUX4] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    motorPowerRange = 255;
  }
  else if (data.ch[TX_AUX4] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    motorPowerRange = 130;
  }
  else
  {
    motorPowerRange = 80;
  }
  //*/

  byte motorOutMode = 0;
  int16_t mix = 1000;
  //* position of AUX4 selects diff motor output modes
  if (data.ch[TX_AUX4] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    motorOutMode = 0;
  }
  else if (data.ch[TX_AUX4] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    motorOutMode = 1;
  }
  else
  {
    motorOutMode = 2;
  }

  if (motorOutMode == 0)
  {

    //*/
    motor1Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
    motor2Val = motor1Val;

    int16_t throttleVal = motor1Val;
    EVERY_N_MILLIS(250)
    {
      // Serial.print("motorPowerRange: " + String(motorPowerRange));
      // Serial.println(", throttleVal: " + String(motor2Val));
    }

    // float mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 750, 250) / 1000.0; // gives a range of .25-.75

    // /*
    // range 0.0-1.0, then an exponent, then map to 250-750
    mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 2000), 0, 2000); // gives a range of 0-2000
    // mix = pow(mix, 1.4);
    // mix = map((mix * 1000.0), 1000, 0, 250, 750) / 1000.0; // reverse the input range because we want to reverse the steering
    // mix = map((mix * 1000.0), 1000, 0, 0, 1000) / 1000.0; // reverse the input range because we want to reverse the steering
    //*/

    motor1Val = (int16_t)((throttleVal * (2000 - mix)) / 2000);
    motor2Val = (int16_t)((throttleVal * mix) / 2000);
  }
  if (motorOutMode == 1)
  {
    motor1Val = 0;
    motor2Val = 0;
    if (abs(data.ch[TX_PITCH] - SBUS_VAL_CENTER) > 200)
    {
      // if forward or backward: move linearly (no mixing)

      motor1Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
      motor2Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
    }
    else
    {
      // if left/right: only left or right, no mixing
      if (data.ch[TX_ROLL] < (SBUS_VAL_CENTER - 150)) // left
      {
        motor2Val = 0;
        motor1Val = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER, SBUS_VAL_MIN, 0, motorPowerRange), 0, 255);
      }
      if (data.ch[TX_ROLL] > (SBUS_VAL_CENTER + 150)) // right
      {
        motor2Val = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER, SBUS_VAL_MAX, 0, motorPowerRange), 0, 255);
        motor1Val = 0;
      }
    }
  }
  if (motorOutMode == 2)
  {
    motor1Val = 0;
    motor2Val = 0;
    if (abs(data.ch[TX_PITCH] - SBUS_VAL_CENTER) > 200)
    {
      // if forward or backward: move linearly (no mixing)

      motor1Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
      motor2Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
    }
    else
    {
      motor1Val = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
      motor2Val = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, motorPowerRange, -motorPowerRange), -255, 255);
    }
  }

  /*
  doSineMovement = (data.ch[TX_AUX4] < SBUS_SWITCH_MIN_THRESHOLD) ? true : false;
  doSineMovement = false; // override

  // only applies if doSineMovement is true
  // sinCounterIncrement = map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 200, 1000) / 5000.0;
  sinCounterIncrement = 550 / 5000.0; // override for testing
  float sinMulFactor = .9;
  sinMulFactor = constrain(map(data.ch[TX_AUX3], SBUS_VAL_MIN, SBUS_VAL_MAX, 450, 900), 450, 900) / 1000.0;
  float sinMult1 = sin(sinCounter) * sinMulFactor + (1.0 - sinMulFactor);
  float sinMult2 = sin(sinCounter + PI) * sinMulFactor + (1.0 - sinMulFactor);
  sinCounter += sinCounterIncrement;
  // Serial.println("sinCounterIncrement: " + String(sinCounterIncrement));

  if (doSineMovement)
  {
    float ampMul = constrain(map(data.ch[TX_AUX4], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 0, 1000) / 1000.0;
    motor1Val *= (sinMult1 * ampMul) + (1.0 - ampMul);
    motor2Val *= (sinMult2 * ampMul) + (1.0 - ampMul);
  }
  //*/

  motor1Val = constrain(motor1Val, -255, 255);
  motor2Val = constrain(motor2Val, -255, 255);

  EVERY_N_MILLIS(200)
  {
    Serial.println("mix: " + String(mix) + ", motor1Val: " + String(motor1Val) + ", motor2Val: " + String(motor2Val) + ", motor1._pwmVal: " + String(motor1._pwmVal) + ", motor2._pwmVal: " + String(motor2._pwmVal));
  }
}

void driveMotors()
{
  // MOTOR 1
  if (abs(motor1Val) < SBUS_VAL_DEADBAND)
  {
    motor1.motorStop(); // Soft Stop    -no argument
  }
  if (motor1Val < -SBUS_VAL_DEADBAND)
  {
    motor1.motorRev(-motor1Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor1Val > SBUS_VAL_DEADBAND)
  {
    motor1.motorGo(motor1Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }

  // MOTOR 2
  if (abs(motor2Val) < SBUS_VAL_DEADBAND)
  {
    motor2.motorStop(); // Soft Stop    -no argument
  }
  if (motor2Val < -SBUS_VAL_DEADBAND)
  {
    motor2.motorRev(-motor2Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor2Val > SBUS_VAL_DEADBAND)
  {
    motor2.motorGo(motor2Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
}