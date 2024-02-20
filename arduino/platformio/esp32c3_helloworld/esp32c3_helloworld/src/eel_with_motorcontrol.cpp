#include <Arduino.h>
#include <ESP32MX1508.h>
#include <Adafruit_BusIO_Register.h>
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

#define PIN_MOTOR1_A D5
#define PIN_MOTOR1_B D4
#define CH_MOTOR1_1 2 // 16 Channels (0-15) are availible
#define CH_MOTOR1_2 3 // Make sure each pin is a different channel and not in use by other PWM devices (servos, LED's, etc)

#define PIN_MOTOR2_A D2
#define PIN_MOTOR2_B D3
#define CH_MOTOR2_1 4 // 16 Channels (0-15) are availible
#define CH_MOTOR2_2 5 // Make sure each pin is a different channel and not in use by other PWM devices (servos, LED's, etc)

// Optional Parameters
#define RES 8     // Resolution in bits:  8 (0-255),  12 (0-4095), or 16 (0-65535)
#define FREQ 5000 // PWM Frequency in Hz

MX1508 motorA(PIN_MOTOR1_A, PIN_MOTOR1_B, CH_MOTOR1_1, CH_MOTOR1_2, 8, 2500); // Default-  8 bit resoluion at 2500 Hz
MX1508 motorB(PIN_MOTOR2_A, PIN_MOTOR2_B, CH_MOTOR2_1, CH_MOTOR2_2, 8, 2500); // Default-  8 bit resoluion at 2500 Hz

u_long sbusPrevPacketTime;
bool sbusLost = false;

#define SBUS_VAL_MIN 191
#define SBUS_VAL_MAX 1793
#define SBUS_VAL_CENTER 988
#define SBUS_VAL_DEADBAND 5
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
  STATE_2
};
HEAD_STATE headState = STATE_1;

enum BODY_STATE
{
  NOOD1,
  NOOD2,
  BOTH_NOODS
};
BODY_STATE bodyState = BOTH_NOODS;

#define SBUS_PACKET_PRINT_INTERVAL 100 // ms
u_long sbusPacketPrintPrevTime = 0;

float sinCounter = 0.0;
float sinCounterIncrement = 0.05;

bool doSineMovement = false;

byte hVal = 0;
byte vVal = 0;

int16_t motor1Val;
int16_t motor2Val;

// define methods
void driveMotors();
void parseSBUS(bool serialPrint);

void setup()
{
  Serial.begin(115200);

  /* Begin the SBUS communication */
  sbus_rx.Begin();
  // sbus_tx.Begin();

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;

  analogWriteResolution(8);
  analogWriteFrequency(2500);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  delay(10);
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show(); // Send the updated pixel colors to the hardware.

  Serial.println("Hello World!");
}

void loop()
{
  // read SBUS
  parseSBUS(true);

  byte motorPowerRange = 255;
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

  motor1Val = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
  motor2Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);

  doSineMovement = (data.ch[TX_AUX4] < SBUS_SWITCH_MIN_THRESHOLD) ? true : false;
  doSineMovement = false; // override

  int16_t throttleVal = motor2Val;
  EVERY_N_MILLIS(250)
  {
    // Serial.print("motorPowerRange: " + String(motorPowerRange));
    // Serial.println(", throttleVal: " + String(motor2Val));
  }

  // float mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 750, 250) / 1000.0; // gives a range of .25-.75

  // /*
  // range 0.0-1.0, then an exponent, then map to 250-750
  float mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 0, 1000) / 1000.0; // gives a range of 0-1.0
  // mix = pow(mix, 1.4);
  // mix = map((mix * 1000.0), 1000, 0, 250, 750) / 1000.0; // reverse the input range because we want to reverse the steering
  mix = map((mix * 1000.0), 1000, 0, 0, 1000) / 1000.0; // reverse the input range because we want to reverse the steering
  //*/

  motor1Val = (throttleVal * (1.0 - mix)) * 2;
  motor2Val = (throttleVal * mix) * 2;

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

  motor1Val = constrain(motor1Val, -255, 255);
  motor2Val = constrain(motor2Val, -255, 255);

  driveMotors();

  // delay a little.
  delay(1000 / 200);
}

void parseSBUS(bool serialPrint)
{
  if (sbus_rx.Read())
  {
    sbusPrevPacketTime = millis();
    if (sbusLost)
    {
      Serial.println("Regained SBUS connection");
      sbusLost = false;
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
    if (!sbusLost)
    {
      Serial.print("Lost SBUS connection >> setting throttle/pitch/roll to ");
      Serial.print((SBUS_VAL_MIN + SBUS_VAL_MAX) / 2);
      Serial.println(" and yaw to 0 ");
      sbusLost = true;
    }
  }

  if (sbusLost)
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
}

void updateHeadBodyState()
{
  if (data.ch[TX_AUX1] < SBUS_SWITCH_MIN_THRESHOLD)
  {
    headState = STATE_1;
  }
  else
  {
    headState = STATE_2;
  }

  if (data.ch[TX_AUX2] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    bodyState = NOOD1;
  }
  else if (data.ch[TX_AUX2] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    bodyState = NOOD2;
  }
  else
  {
    bodyState = BOTH_NOODS;
  }
}

void driveMotors()
{
  // MOTOR 1
  if (abs(motor1Val) < SBUS_VAL_DEADBAND)
  {
    motorA.motorStop(); // Soft Stop    -no argument
  }
  if (motor1Val < -SBUS_VAL_DEADBAND)
  {
    motorA.motorRev(-motor1Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor1Val > SBUS_VAL_DEADBAND)
  {
    motorA.motorGo(motor1Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }

  // MOTOR 2
  if (abs(motor2Val) < SBUS_VAL_DEADBAND)
  {
    motorB.motorStop(); // Soft Stop    -no argument
  }
  if (motor2Val < -SBUS_VAL_DEADBAND)
  {
    motorB.motorRev(-motor2Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor2Val > SBUS_VAL_DEADBAND)
  {
    motorB.motorGo(motor2Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
}