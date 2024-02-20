#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>

#include "sbus.h"
#include <HardwareSerial.h>

HardwareSerial MySerial0(0);

/* SBUS object, reading SBUS */
bfs::SbusRx sbus_rx(&MySerial0, D7, D6, true, false);

/* SBUS data */
bfs::SbusData data;

#define NUM_LEDS 2
#define DATA_PIN D8

u_long sbusPrevPacketTime;
bool sbusLost = false;

#define SBUS_VAL_MIN 176
#define SBUS_VAL_MAX 1800
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

#define SBUS_PACKET_PRINT_INTERVAL 100 // ms
u_long sbusPacketPrintPrevTime = 0;

CRGB leds[NUM_LEDS];

void setup()
{
  Serial.begin(115200);

  /* Begin the SBUS communication */
  sbus_rx.Begin();
  // sbus_tx.Begin();

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(128);

  Serial.println("Hello World!");
}

byte cycle = 0;
void loop()
{
  // read SBUS
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
      if (Serial)
      {
        for (int8_t i = 0; i < data.NUM_CH; i++)
        {
          Serial.print(data.ch[i]);
          Serial.print("\t");
        }
        Serial.println();
      }

      sbusPacketPrintPrevTime = millis();
    }
  }

  // if SBUS lost, reset the channels
  if (millis() - sbusPrevPacketTime > SBUS_LOST_TIMEOUT)
  {
    if (!sbusLost)
    {
      Serial.print("Lost SBUS connection >> setting first 4 channels to ");
      Serial.println((SBUS_VAL_MIN + SBUS_VAL_MAX) / 2);
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
      }
    }
  }

  EVERY_N_SECONDS(1)
  {
    cycle++;
    cycle %= 4;
  }
  if (data.ch[TX_AUX1] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    cycle = 4;
  }

  switch (cycle)
  {
  case 0:
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Red;
    }
    break;
  case 1:
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Green;
    }
    break;
  case 2:
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Blue;
    }
    break;
  case 3:
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB::Black;
    }
    break;
  case 4:
    byte hVal = constrain(map(data.ch[TX_YAW], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 250), 0, 250);
    byte vVal = constrain(map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 225), 0, 225);
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CHSV(hVal, 240, 25 + vVal);
    }
    break;
  }
  FastLED.show();

  delay(1000 / 200);
}