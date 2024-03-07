#include <Adafruit_NeoPixel.h>

// NeoPixel
#define PIN D9
#define NUMPIXELS 3

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void initPixels();
void setRGBLights();
void updateHeadLighting();

void initPixels()
{
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  delay(10);
  pixels.clear();
  pixels.show();
}

// led 1 and 3 = eyes
// led 2 = mouth
void updateHeadLighting()
{
  uint8_t audioVal = constrain(map(data.ch[TX_YAW], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 255), 0, 255);
  switch (headState)
  {
  case STATE_1: // tmp: off
    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // left eye
    pixels.setPixelColor(1, pixels.Color(0, 0, 0)); // mouth
    pixels.setPixelColor(2, pixels.Color(0, 0, 0)); // right eye
    // pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // left eye
    // pixels.setPixelColor(1, pixels.Color(255, audioVal, audioVal)); // mouth
    // pixels.setPixelColor(2, pixels.Color(audioVal, 255, audioVal)); // right eye
    break;
  case STATE_2: // tmp: green
    pixels.setPixelColor(0, pixels.Color(0, audioVal, 0)); // left eye
    pixels.setPixelColor(1, pixels.Color(0, audioVal, 0));      // mouth
    pixels.setPixelColor(2, pixels.Color(0, audioVal, 0)); // right eye
    // pixels.setPixelColor(0, pixels.Color(audioVal, audioVal, audioVal)); // left eye
    // pixels.setPixelColor(1, pixels.Color(255, audioVal, audioVal));      // mouth
    // pixels.setPixelColor(2, pixels.Color(audioVal, audioVal, audioVal)); // right eye
    break;
  case STATE_3: // tmp: white
    pixels.setPixelColor(0, pixels.Color(audioVal, audioVal, audioVal)); // left eye
    pixels.setPixelColor(1, pixels.Color(audioVal, audioVal, audioVal));      // mouth
    pixels.setPixelColor(2, pixels.Color(audioVal, audioVal, audioVal)); // right eye
    // pixels.setPixelColor(0, pixels.Color(audioVal, audioVal, audioVal)); // left eye
    // pixels.setPixelColor(1, pixels.Color(255, audioVal, audioVal));      // mouth
    // pixels.setPixelColor(2, pixels.Color(audioVal, audioVal, audioVal)); // right eye
    break;
  }
  pixels.show(); // Send the updated pixel colors to the hardware.
}


void setRGBLights()
{
  // ??
}

