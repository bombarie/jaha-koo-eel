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
void updateHeadBodyLights_disconnected();
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
  // read SBUS
  parseSBUS(printSbusData);

  updateSerialIO();

  updateHeadBodyState();

  // determine how to set the head and body lights
  switch (connectionState)
  {
  case DISCONNECTED:
    resetSbusData();
    updateHeadBodyLights_disconnected();
    break;
  case CONNECTION_ESTABLISHED:
    // maybe do some temporary transition lighting here from disconnected to connected, but for now just go to connected
    connectionState = CONNECTED;
    break;
  case CONNECTION_LOST:
    // maybe do some temporary transition lighting here from connected to disconnected, but for now just go to disconnected
    connectionState = DISCONNECTED;
    break;
  case CONNECTED:
    updateBodyLightValues();
    updateHeadLightValues();
    break;
  }

  setBodyLights();
  setHeadLights();

  calcMotorValues();
  driveMotors();

  // EVERY_N_SECONDS(1)
  // {
  //   Serial.println("connectionState: " + String(connectionState) + ", headState: " + String(headState));
  // }

  // delay a little.
  delay(1000 / 200);
}

void updateHeadBodyState()
{
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

  // override head state if aux1 is low
  if (data.ch[TX_AUX1] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    headState = STATE_4;
  }
}

void updateHeadBodyLights_disconnected()
{
  // set noods off
  noodAvgVals[0] = 0;
  noodAvgVals[1] = 0;
  noodAvgVals[2] = 0;
  noodAvgVals[3] = 0;

  // blink rgb leds red
  if ((millis() / 500) % 2 == 0) {
    rgbLeds[RIGHT_EYE] = pixels.Color(255, 0, 0);
    rgbLeds[MOUTH] = pixels.Color(255, 0, 0);
    rgbLeds[LEFT_EYE] = pixels.Color(255, 0, 0);
  } else {
    rgbLeds[RIGHT_EYE] = pixels.Color(0, 0, 0);
    rgbLeds[MOUTH] = pixels.Color(0, 0, 0);
    rgbLeds[LEFT_EYE] = pixels.Color(0, 0, 0);
  }
}
