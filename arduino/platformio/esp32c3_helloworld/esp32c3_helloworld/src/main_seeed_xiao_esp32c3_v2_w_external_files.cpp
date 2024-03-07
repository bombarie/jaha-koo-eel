#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>

#include "external_classes/vars.h"
#include "external_classes/elrs_rx.h"
#include "external_classes/motor_control.h"
#include "external_classes/neopixels.h"
#include "external_classes/nood_control.h"
#include "external_classes/serial_io.h"


float sinCounter = 0.0;
float sinCounterIncrement = 0.05;

bool doSineMovement = false;

byte hVal = 0;
byte vVal = 0;


// define methods
void setLights_disconnected();
void updateHeadBodyState();

void setup()
{
  Serial.begin(115200);

  initELRSRX();
  initNoods();
  initMotors();
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


void setLights_disconnected()
{
  // ??
}

