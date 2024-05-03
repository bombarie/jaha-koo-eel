#include "sbus.h"
#include <HardwareSerial.h>

HardwareSerial MySerial0(0);

/* SBUS object, reading SBUS */
bfs::SbusRx sbus_rx(&MySerial0, D7, D6, true, false);

/* SBUS data */
bfs::SbusData data;

#define SBUS_PACKET_PRINT_INTERVAL 20 // ms (originally 100ms)
u_long sbusPacketPrintPrevTime = 0;

u_long sbusPrevPacketTime;
bool sbusLost = false;

#define SBUS_VAL_MIN 176 // 191
#define SBUS_VAL_MAX 1808 // 1793
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

void initELRSRX();
void resetSbusData();
void parseSBUS(bool serialPrint);

void initELRSRX() {
  /* Begin the SBUS communication */
  
  // TODO -> define the RX as input_pullup, so that we might prevent / circumvent the bootloader mode error?
  pinMode(D7, INPUT_PULLUP); // pull up the RX pin

  sbus_rx.Begin();
  // sbus_tx.Begin();

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;
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
