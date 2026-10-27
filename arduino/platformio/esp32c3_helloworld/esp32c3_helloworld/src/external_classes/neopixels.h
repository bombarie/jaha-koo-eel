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
void setHeadLights();
void updateHeadLightValues();

uint32_t rgbLeds[3] = {0, 0, 0};

enum EYE_COLORS
{
    OFF = 0,
    RED = 1,
    GREEN = 2,
    BLUE = 3,
    ORANGE = 4,
    WHITE = 5
};

#define LEFT_EYE 0
#define RIGHT_EYE 2
#define MOUTH 1

#define hue360ToNeopixelHue(hue) hue * 182

void initPixels()
{
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    delay(10);
    pixels.clear();
    pixels.show();
}

// led 1 and 3 = eyes
// led 2 = mouth
uint8_t eyesBaseVal = 32;
uint8_t mouthBaseVal = 32;
void updateHeadLightValues()
{
    uint8_t audioVal = constrain(map(data.ch[TX_YAW], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 255), 0, 255);

    rgbLeds[RIGHT_EYE] = pixels.Color(0, eyesBaseVal + map(audioVal, 0, 255, 0, (255 - eyesBaseVal)), 0);
    rgbLeds[LEFT_EYE] = pixels.Color(0, eyesBaseVal + map(audioVal, 0, 255, 0, (255 - eyesBaseVal)), 0);

    switch (headState)
    {
    case STATE_1:
        /* tmp: all off
        rgbLeds[RIGHT_EYE] = pixels.Color(0, 0, 0);
        rgbLeds[MOUTH] = pixels.Color(0, 0, 0);
        rgbLeds[LEFT_EYE] = pixels.Color(0, 0, 0);
        // pixels.setPixelColor(LEFT_EYE, pixels.Color(0, 0, 0));
        // pixels.setPixelColor(RIGHT_EYE, pixels.Color(0, 0, 0));
        // pixels.setPixelColor(MOUTH, pixels.Color(0, 0, 0));
        /*/
        rgbLeds[MOUTH] = pixels.Color(0, 0, 0);

        // pixels.setPixelColor(RIGHT_EYE, pixels.Color(255, 0, 0));
        // pixels.setPixelColor(MOUTH, pixels.Color(255, audioVal, audioVal));
        // pixels.setPixelColor(LEFT_EYE, pixels.Color(audioVal, 255, audioVal));
        //*/
        break;
    case STATE_2:
        /* tmp: all green
        pixels.setPixelColor(LEFT_EYE, pixels.Color(0, audioVal, 0));
        pixels.setPixelColor(RIGHT_EYE, pixels.Color(0, audioVal, 0));
        pixels.setPixelColor(MOUTH, pixels.Color(0, audioVal, 0));
        /*/
        // rgbLeds[MOUTH] = pixels.Color(255, audioVal, audioVal);
        rgbLeds[MOUTH] = pixels.ColorHSV(hue360ToNeopixelHue(315), 127, mouthBaseVal);

        // pixels.setPixelColor(RIGHT_EYE, pixels.Color(audioVal, audioVal, audioVal));
        // pixels.setPixelColor(MOUTH, pixels.Color(255, audioVal, audioVal));
        // pixels.setPixelColor(LEFT_EYE, pixels.Color(audioVal, audioVal, audioVal));
        //*/
        break;
    case STATE_3:
        /* tmp: all white
        pixels.setPixelColor(LEFT_EYE, pixels.Color(audioVal, audioVal, audioVal));
        pixels.setPixelColor(RIGHT_EYE, pixels.Color(audioVal, audioVal, audioVal));
        pixels.setPixelColor(MOUTH, pixels.Color(audioVal, audioVal, audioVal));
        /*/
        // rgbLeds[MOUTH] = pixels.Color(255, audioVal, audioVal);
        rgbLeds[MOUTH] = pixels.ColorHSV(hue360ToNeopixelHue(315), 127, mouthBaseVal + map(audioVal, 0, 255, 0, (255 - mouthBaseVal)));

        // pixels.setPixelColor(RIGHT_EYE, pixels.Color(audioVal, audioVal, audioVal));
        // pixels.setPixelColor(MOUTH, pixels.Color(255, audioVal, audioVal));
        // pixels.setPixelColor(LEFT_EYE, pixels.Color(audioVal, audioVal, audioVal));
        //*/
        break;
    case STATE_4:
        rgbLeds[RIGHT_EYE] = pixels.Color(0, eyesBaseVal, 0);
        rgbLeds[LEFT_EYE] = pixels.Color(0, eyesBaseVal, 0);
        rgbLeds[MOUTH] = pixels.ColorHSV(hue360ToNeopixelHue(315), 127, mouthBaseVal + map(audioVal, 0, 255, 0, (255 - mouthBaseVal)));

        break;
    }
    // pixels.show(); // Send the updated pixel colors to the hardware.
}

void setHeadLights()
{
    for (int i = 0; i < 3; i++)
    {
        pixels.setPixelColor(i, rgbLeds[i]);
    }
    pixels.show(); // Send the updated pixel colors to the hardware.
}
