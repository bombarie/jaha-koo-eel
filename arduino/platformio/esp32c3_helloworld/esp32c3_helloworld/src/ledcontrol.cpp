#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>

#define NUM_LEDS 2
#define DATA_PIN D0

CRGB leds[NUM_LEDS];

void setup()
{
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

byte cycle = 0;
void loop()
{
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
  }
  FastLED.show();

  delay(1000 / 200);
  EVERY_N_SECONDS(1)
  {
    cycle++;
    cycle %= 4;
  }
}