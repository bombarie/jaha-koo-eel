#include <Arduino.h>
#include <Wire.h>
#include "sbus.h"
#include <HardwareSerial.h>
#include <FastLED.h>

HardwareSerial MySerial0(0);

/* SBUS object, reading SBUS */
bfs::SbusRx sbus_rx(&MySerial0, D7, D6, true, false);

/* SBUS data */
bfs::SbusData data;

#define n00d_1a_Pin D1
#define n00d_2a_Pin D0
#define n00d_2b_Pin D8
#define n00d_1b_Pin D10

u_long sbusPrevPacketTime;
bool sbusLost = false;

#define SBUS_VAL_MIN 191
#define SBUS_VAL_MAX 1793
#define SBUS_VAL_CENTER 988
#define SBUS_VAL_DEADBAND 5
#define SBUS_LOST_TIMEOUT 100
#define SBUS_SWITCH_MIN 191
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

int16_t n00d1a, n00d1b, n00d2a, n00d2b;
uint16_t throttle, throttleAdjusted;

int16_t n00dsAvg[] = {0, 0, 0, 0};
uint16_t n00dSegmentIdentifiers[] = {512, 640, 768, 896};

void parseSBUS(bool serialPrint);
void updateBodyValues();
void updateBodyLighting();
void updateSerialIO();
void checkIncomingSerial();
void setn00d(uint8_t pin, uint8_t val);

void setup()
{
  Serial.begin(115200);

  analogWriteResolution(8);
  analogWriteFrequency(2500);

  /* Begin the SBUS communication */
  sbus_rx.Begin();
  // sbus_tx.Begin();

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;

  pinMode(n00d_1a_Pin, OUTPUT);
  pinMode(n00d_1b_Pin, OUTPUT);
  pinMode(n00d_2a_Pin, OUTPUT);
  pinMode(n00d_2b_Pin, OUTPUT);

  setn00d(n00d_1a_Pin, 0);
  setn00d(n00d_1b_Pin, 0);
  setn00d(n00d_2a_Pin, 0);
  setn00d(n00d_2b_Pin, 0);

  Serial.println("Hello World!");
}

void loop()
{
  // read SBUS
  parseSBUS(false);

  updateBodyValues();
  updateBodyLighting();

  updateSerialIO();

  delay(1000 / 300);
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

  n00dsAvg[0] = 0.85 * n00dsAvg[0] + 0.15 * n00d1a;
  n00dsAvg[1] = 0.85 * n00dsAvg[1] + 0.15 * n00d1b;
  n00dsAvg[2] = 0.85 * n00dsAvg[2] + 0.15 * n00d2a;
  n00dsAvg[3] = 0.85 * n00dsAvg[3] + 0.15 * n00d2b;
}

void updateBodyLighting()
{
  // setn00d(n00d_1a_Pin, n00d1a);
  // setn00d(n00d_1b_Pin, n00d1b);
  // setn00d(n00d_2a_Pin, n00d2a);
  // setn00d(n00d_2b_Pin, n00d2b);

  setn00d(n00d_1a_Pin, n00dsAvg[0]);
  setn00d(n00d_1b_Pin, n00dsAvg[1]);
  setn00d(n00d_2a_Pin, n00dsAvg[2]);
  setn00d(n00d_2b_Pin, n00dsAvg[3]);
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

void setn00d(uint8_t pin, uint8_t val)
{
  analogWrite(pin, (255 - val));
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
