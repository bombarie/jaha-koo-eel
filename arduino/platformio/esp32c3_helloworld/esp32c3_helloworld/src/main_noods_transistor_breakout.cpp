#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>

// NeoPixel
#define PIN D9
#define NUMPIXELS 3

#define n00d_1a_Pin D1
#define n00d_2a_Pin D0
#define n00d_2b_Pin D8
#define n00d_1b_Pin D10


float sinVal = 0.0;
float sinIncrement = 0.082;
float phase_offset = .35;

void doSineAnimation1();
void doSineAnimation2();
void doAllOn();
void doAllOff();
void doFlashing();
void doFlash2();
void doTaperOff1();

#define n00d_on 255
#define n00d_off 0

void setup()
{
  Serial.begin(115200);

  analogWriteResolution(8);
  analogWriteFrequency(2500);

  // debug - prevent motor in the snake from spining
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  digitalWrite(D4, LOW);
  digitalWrite(D5, HIGH);
  

  pinMode(n00d_1a_Pin, OUTPUT);
  pinMode(n00d_1b_Pin, OUTPUT);
  pinMode(n00d_2a_Pin, OUTPUT);
  pinMode(n00d_2b_Pin, OUTPUT);

  delay(10000);

  Serial.println("Hello World!");
}

byte animationState = 0;
void loop()
{
  EVERY_N_SECONDS(10)
  {
    animationState++;
    animationState %= 6;
  }

  switch (animationState)
  {
  case 0:
    sinIncrement = 0.082;
    phase_offset = .35;
    doSineAnimation1();
    break;
  case 1:
    sinIncrement = 0.060;
    phase_offset = 0.85 * PI;
    doSineAnimation2();
    break;
  case 2:
    doAllOn();
    break;
  case 5:
    doFlashing();
    break;
  case 6:
    doFlash2();
    break;
  case 7:
    doTaperOff1();
    break;
  }

  delay(1000 / 60);
}

void doSineAnimation1()
{
  sinVal += sinIncrement;

  float nood1Sinval = sin(sinVal);
  float nood2Sinval = sin(sinVal + ((1 * phase_offset) * TWO_PI));
  float nood3Sinval = sin(sinVal + ((2 * phase_offset) * TWO_PI));
  float nood4Sinval = sin(sinVal + ((3 * phase_offset) * TWO_PI));

  byte nood1_pwmVal = (byte)(nood1Sinval * 110.0 + 145.0);
  byte nood2_pwmVal = (byte)(nood2Sinval * 110.0 + 145.0);
  byte nood3_pwmVal = (byte)(nood3Sinval * 110.0 + 145.0);
  byte nood4_pwmVal = (byte)(nood4Sinval * 110.0 + 145.0);

  analogWrite(n00d_1a_Pin, nood1_pwmVal);
  analogWrite(n00d_1b_Pin, nood2_pwmVal);
  analogWrite(n00d_2a_Pin, nood3_pwmVal);
  analogWrite(n00d_2b_Pin, nood4_pwmVal);
}

void doSineAnimation2()
{
  sinVal += sinIncrement;

  float nood1Sinval = sin(sinVal + ((1 * phase_offset) * TWO_PI));
  float nood2Sinval = sin(sinVal);
  float nood3Sinval = nood1Sinval;
  float nood4Sinval = nood2Sinval;

  byte nood1_pwmVal = (byte)(nood1Sinval * 110.0 + 145.0);
  byte nood2_pwmVal = (byte)(nood2Sinval * 110.0 + 145.0);
  byte nood3_pwmVal = (byte)(nood3Sinval * 110.0 + 145.0);
  byte nood4_pwmVal = (byte)(nood4Sinval * 110.0 + 145.0);

  analogWrite(n00d_1a_Pin, nood1_pwmVal);
  analogWrite(n00d_1b_Pin, nood2_pwmVal);
  analogWrite(n00d_2a_Pin, nood3_pwmVal);
  analogWrite(n00d_2b_Pin, nood4_pwmVal);
}

void doAllOn()
{
  analogWrite(n00d_1a_Pin, n00d_on);
  analogWrite(n00d_1b_Pin, n00d_on);
  analogWrite(n00d_2a_Pin, n00d_on);
  analogWrite(n00d_2b_Pin, n00d_on);
}

void doFlashing()
{
  doAllOff();

  // flash n00d 1a
  analogWrite(n00d_1a_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_1a_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_1a_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_1a_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_1a_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_1a_Pin, n00d_off);
  delay(200);

  // flash n00d 1b
  analogWrite(n00d_1b_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_1b_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_1b_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(200);

  // flash n00d 2a
  analogWrite(n00d_2a_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_2a_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_2a_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_2a_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_2a_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_2a_Pin, n00d_off);
  delay(200);

  // flash n00d 2b
  analogWrite(n00d_2b_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_2b_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(18);
  analogWrite(n00d_2b_Pin, n00d_on);
  delay(18);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(200);
}

// all n00ds off
void doAllOff()
{
  analogWrite(n00d_1a_Pin, n00d_off);
  analogWrite(n00d_1b_Pin, n00d_off);
  analogWrite(n00d_2a_Pin, n00d_off);
  analogWrite(n00d_2b_Pin, n00d_off);
}
void doFlash2()
{
  doAllOff();

  // flash n00d 1a and 1b intermittently
  analogWrite(n00d_1a_Pin, n00d_on);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(25);
  analogWrite(n00d_1a_Pin, n00d_off);
  analogWrite(n00d_1b_Pin, n00d_on);
  delay(25);
  analogWrite(n00d_1a_Pin, n00d_on);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(25);
  analogWrite(n00d_1a_Pin, n00d_off);
  analogWrite(n00d_1b_Pin, n00d_on);
  delay(25);
  analogWrite(n00d_1a_Pin, n00d_on);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(25);
  analogWrite(n00d_1a_Pin, n00d_off);
  analogWrite(n00d_1b_Pin, n00d_on);
  delay(25);
  analogWrite(n00d_1a_Pin, n00d_off);
  analogWrite(n00d_1b_Pin, n00d_off);
  delay(200);

  // flash n00d 2a and 2b intermittently
  analogWrite(n00d_2a_Pin, n00d_on);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(25);
  analogWrite(n00d_2a_Pin, n00d_off);
  analogWrite(n00d_2b_Pin, n00d_on);
  delay(25);
  analogWrite(n00d_2a_Pin, n00d_on);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(25);
  analogWrite(n00d_2a_Pin, n00d_off);
  analogWrite(n00d_2b_Pin, n00d_on);
  delay(25);
  analogWrite(n00d_2a_Pin, n00d_on);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(25);
  analogWrite(n00d_2a_Pin, n00d_off);
  analogWrite(n00d_2b_Pin, n00d_on);
  delay(25);
  analogWrite(n00d_2a_Pin, n00d_off);
  analogWrite(n00d_2b_Pin, n00d_off);
  delay(200);
}

void doTaperOff1()
{
  doAllOff();

  for (int i = n00d_off; i < n00d_on; i++)
  {
    analogWrite(n00d_1a_Pin, i);
    delay(2);
  }

  // doAllOff();

  for (int i = n00d_off; i < n00d_on; i++)
  {
    analogWrite(n00d_1b_Pin, i);
    delay(2);
  }

  // doAllOff();

  for (int i = n00d_off; i < n00d_on; i++)
  {
    analogWrite(n00d_2a_Pin, i);
    analogWrite(n00d_2b_Pin, n00d_on - i);
    delay(2);
  }

  // doAllOff();

  for (int i = n00d_off; i < n00d_on; i++)
  {
    analogWrite(n00d_2a_Pin, n00d_on - i);
    analogWrite(n00d_2b_Pin, i);
    delay(2);
  }

  // doAllOff();

  for (int i = n00d_off; i < n00d_on; i++)
  {
    analogWrite(n00d_1a_Pin, i);
    analogWrite(n00d_1b_Pin, n00d_on - i);
    analogWrite(n00d_2a_Pin, i);
    analogWrite(n00d_2b_Pin, n00d_on - i);
    delay(2);
  }

  // doAllOff();

  for (int i = n00d_off; i < n00d_on; i++)
  {
    analogWrite(n00d_1a_Pin, i);
    analogWrite(n00d_1b_Pin, n00d_on - i);
    analogWrite(n00d_2a_Pin, i);
    analogWrite(n00d_2b_Pin, n00d_on - i);
    delay(2);
  }
}