#include <Arduino.h>

void setup()
{
  Serial.begin(115200);

  delay(2000);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println("SETUP is done");
}

// the loop function runs over and over again forever
void loop()
{
  Serial.println("HIgH");
  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(1000);                     // wait for a second
  Serial.println("LOW");
  digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  delay(1000);                    // wait for a second
}
