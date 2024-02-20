#include <Arduino.h>
#include <Wire.h>

// Simple sketch testing analogwrite for the noods.
// ... it works :)

#define nood1a D1
#define nood1b D10

// to prevent the motors from running
#define motor1a_pin D5
#define motor1b_pin D4
#define motor2a_pin D3
#define motor2b_pin D2

void setup()
{
  Serial.begin(115200);

  // to prevent the motors from running
  pinMode(motor1a_pin, OUTPUT);
  pinMode(motor1b_pin, OUTPUT);
  pinMode(motor2a_pin, OUTPUT);
  pinMode(motor2b_pin, OUTPUT);

  // to prevent the motors from running
  analogWrite(motor1a_pin, 127);
  analogWrite(motor1b_pin, 127);
  analogWrite(motor2a_pin, 127);
  analogWrite(motor2b_pin, 127);

  pinMode(nood1a, OUTPUT);
  pinMode(nood1b, OUTPUT);

  Serial.println("main_ledcontrol_noods - hello");
}

void loop()
{
  analogWrite(nood1a, 255);
  analogWrite(nood1b, 220);
  delay(200);
  analogWrite(nood1b, 127);
  delay(200);
  analogWrite(nood1b, 0);
  delay(500);

  analogWrite(nood1b, 255);
  analogWrite(nood1a, 220);
  delay(200);
  analogWrite(nood1a, 127);
  delay(200);
  analogWrite(nood1a, 0);
  delay(500);
}