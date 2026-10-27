#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>

#include "external_classes/vars.h"
#include "external_classes/elrs_rx.h"
#include "external_classes/motor_control.h"
#include "external_classes/neopixels.h"
#include "external_classes/nood_control.h"
#include "external_classes/serial_io.h"

/*

FYI: Serial Monitor commands:
1 -> outputting only nood1a
2 -> outputting only nood1b
3 -> outputting only nood2a
4 -> outputting only nood2b
a -> outputting all noods (DEFAULT)
s -> print incoming sbus data
x -> print nothing (no noods and no sbus data)

*/

/****************************************************************************************************************
 
▗▖ ▗▖▗▖ ▗▖▗▄▄▄▖ ▗▄▄▖▗▖ ▗▖    ▗▄▄▄▖▗▄▄▄▖▗▖   
▐▌ ▐▌▐▌ ▐▌  █  ▐▌   ▐▌ ▐▌    ▐▌   ▐▌   ▐▌   
▐▌ ▐▌▐▛▀▜▌  █  ▐▌   ▐▛▀▜▌    ▐▛▀▀▘▐▛▀▀▘▐▌   
▐▙█▟▌▐▌ ▐▌▗▄█▄▖▝▚▄▄▖▐▌ ▐▌    ▐▙▄▄▖▐▙▄▄▖▐▙▄▄▖

Select which Eel body you are using by changing the EEL_BODY define below.
FYI: Eel #3 is the one developed by Daan & Nathan

****************************************************************************************************************/
#define EEL_BODY 1

float sinCounter = 0.0;
float sinCounterIncrement = 0.05;

bool doSineMovement = false;

byte hVal = 0;
byte vVal = 0;

// define methods
void blinkHeadLights(uint16_t blinkDelay, EYE_COLORS color);
void updateHeadBodyState();

void setup()
{
  Serial.begin(115200);

  initELRSRX();
  initNoods();

  if (EEL_BODY == 1 || EEL_BODY == 2)
  {
    initMotors(false, 1.0, false, false);
  }
  if (EEL_BODY == 3)
  {
    initMotors(true, 0.85, 0.22, false, false);
  }

  initPixels();

  Serial.println("Hello World!");
}

void loop()
{
  EVERY_N_MILLIS(1000 / 500)
  {
    // read SBUS
    parseSBUS(printSbusData);
  }

  EVERY_N_MILLIS(1000 / 300)
  {
    updateSerialIO();

    updateHeadBodyState();

    // determine how to set the head and body lights
    switch (connectionState)
    {
    case DISCONNECTED:
      resetSbusData();
      blinkHeadLights(500, EYE_COLORS::RED);
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

      // display blinking red lights if calibrating
      if (calibState != CalibState::CALIB_IDLE || noodZeroBaselineCapturing)
      {
        blinkHeadLights(150, EYE_COLORS::ORANGE);
      }
      else
      {
        updateHeadLightValues();
      }
      break;
    }

    setBodyLights();
    setHeadLights();

    calcMotorValues();
    driveMotors();
  }
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

void blinkHeadLights(uint16_t blinkDelay, EYE_COLORS color)
{
  // set noods off
  noodAvgVals[0] = 0;
  noodAvgVals[1] = 0;
  noodAvgVals[2] = 0;
  noodAvgVals[3] = 0;

  // blink rgb leds red
  if ((millis() / blinkDelay) % 2 == 0)
  {
    switch (color)
    {
    case EYE_COLORS::RED:
      rgbLeds[RIGHT_EYE] = pixels.Color(255, 0, 0);
      rgbLeds[MOUTH] = pixels.Color(255, 0, 0);
      rgbLeds[LEFT_EYE] = pixels.Color(255, 0, 0);
      break;
    case EYE_COLORS::GREEN:
      rgbLeds[RIGHT_EYE] = pixels.Color(0, 255, 0);
      rgbLeds[MOUTH] = pixels.Color(0, 255, 0);
      rgbLeds[LEFT_EYE] = pixels.Color(0, 255, 0);
      break;
    case EYE_COLORS::BLUE:
      rgbLeds[RIGHT_EYE] = pixels.Color(0, 0, 255);
      rgbLeds[MOUTH] = pixels.Color(0, 0, 255);
      rgbLeds[LEFT_EYE] = pixels.Color(0, 0, 255);
      break;
    case EYE_COLORS::ORANGE:
      rgbLeds[RIGHT_EYE] = pixels.Color(255, 165, 0);
      rgbLeds[MOUTH] = pixels.Color(255, 165, 0);
      rgbLeds[LEFT_EYE] = pixels.Color(255, 165, 0);
      break;
    case EYE_COLORS::WHITE:
      rgbLeds[RIGHT_EYE] = pixels.Color(255, 255, 255);
      rgbLeds[MOUTH] = pixels.Color(255, 255, 255);
      rgbLeds[LEFT_EYE] = pixels.Color(255, 255, 255);
      break;
    }
  }
  else
  {
    rgbLeds[RIGHT_EYE] = pixels.Color(0, 0, 0);
    rgbLeds[MOUTH] = pixels.Color(0, 0, 0);
    rgbLeds[LEFT_EYE] = pixels.Color(0, 0, 0);
  }
}
