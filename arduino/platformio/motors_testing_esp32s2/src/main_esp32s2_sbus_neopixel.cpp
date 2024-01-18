#include <Arduino.h>
// #include <SoftwareSerial.h>
#include "sbus.h"
#include <Adafruit_NeoPixel.h>

// SoftwareSerial mySerial(37, 39); // RX, TX

HardwareSerial MySerial0(0);

#define PIN 3       // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 3 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

/* SBUS object, reading SBUS */
bfs::SbusRx sbus_rx(&MySerial0, 39, 37, true, false);
// bfs::SbusRx sbus_rx(&mySerial, 37, 39, true, false);

/* SBUS data */
bfs::SbusData data;

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

// setting PWM properties
const int freq = 500;
const int ledChannel = 1;
const int resolution = 8;

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println("we are in business.....");

  /* Begin the SBUS communication */
  sbus_rx.Begin();

  // by default, let's have the program assume sbus is lost
  sbusPrevPacketTime = -SBUS_LOST_TIMEOUT;

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  Serial.println(".... ready for action");
}

void loop()
{
  // Serial.println(millis());

  pixels.clear(); // Set all pixel colors to 'off'
  uint32_t ledstripVal = 0;
  // ledstripVal = pixels.ColorHSV(neopixelHVal, neopixelSVal, neopixelVVal);
  // ledstripVal = pixels.Color (neopixel_rVal, neopixel_gVal, neopixel_bVal÷);
  if (!sbusLost)
  {
    uint16_t _hue = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 65535), 0, 65535);
    uint8_t _sat = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, 127, 255), 127, 255);
    ledstripVal = pixels.ColorHSV(_hue, _sat, 127);

    // ledstripVal = pixels.Color (0, 127, 0);
  }
  else
  {
    ledstripVal = pixels.Color(127, 0, 0);
  }
  // ledstripVal = pixels.Color (127, 0, 127);
  // hsvVal = pixels.ColorHSV(0, 255, 255);
  pixels.fill(ledstripVal);
  pixels.show(); // Send the updated pixel colors to the hardware.

  //*
  // read SBUS
  if (sbus_rx.Read())
  {
    sbusPrevPacketTime = millis();
    if (sbusLost)
    {
      Serial.println("Regained SBUS connection");
      sbusLost = false;
    }

    data = sbus_rx.data();

    if (millis() - sbusPacketPrintPrevTime > SBUS_PACKET_PRINT_INTERVAL)
    {
      //*
      if (Serial)
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
      Serial.print("Lost SBUS connection >> setting first 4 channels to ");
      Serial.println((SBUS_VAL_MIN + SBUS_VAL_MAX) / 2);
      sbusLost = true;
    }
  }
  //*/

  delay(1000 / 50);
}