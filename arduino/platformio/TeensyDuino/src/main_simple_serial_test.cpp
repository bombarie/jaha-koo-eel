#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

void BlinkLed(byte num) // Basic blink function
{
  for (byte i = 0; i < num; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}

void setup()
{
  Serial.begin(115200);
  // Serial.println("Hello World");

  pinMode(LED_BUILTIN, OUTPUT);

  BlinkLed(2);

  // from https://www.pjrc.com/teensy/teensy31.html
  analogWriteResolution(10); // need 10 bits to be able to encode the n00d protocol

}

void loop()
{
  Serial.println(millis());
  delay(1000);
}

