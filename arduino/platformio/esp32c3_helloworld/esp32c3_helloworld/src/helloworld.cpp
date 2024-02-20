#include <Arduino.h>

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  Serial.println("Hello....");
  delay(1000);
  Serial.println("....World!");
  delay(1000);
}
