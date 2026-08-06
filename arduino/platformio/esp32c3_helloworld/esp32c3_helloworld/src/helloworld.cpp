#include <Arduino.h>

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  Serial.println("Hallo....");
  delay(1000);
  Serial.println("....Welt!");
  delay(1000);
}
