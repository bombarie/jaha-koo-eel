#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#define PIN D8 // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 2 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  Serial.begin(115200);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

byte cycle = 0;
void loop()
{
  pixels.clear(); // Set all pixel colors to 'off'

  //*
  switch (cycle)
  {
  case 0:
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, pixels.Color(150, 0, 0));
    }
    break;
  case 1:
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, pixels.Color(0, 150, 0));
    }
    break;
  case 2:
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, pixels.Color(0, 0, 150));
    }
    break;
  case 3:
    for (int i = 0; i < NUMPIXELS; i++)
    {
      pixels.setPixelColor(i, pixels.Color(150, 150, 0));
    }
    break;
  }
  pixels.show(); // Send the updated pixel colors to the hardware.
  //*/

  delay(1000 / 200);

  EVERY_N_SECONDS(1)
  {
    cycle++;
    cycle %= 4;
  }
}