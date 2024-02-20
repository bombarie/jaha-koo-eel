#include <Arduino.h>
#include <ESP32MX1508.h>
#include <Adafruit_BusIO_Register.h>

#define PINA D5
#define PINB D4

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello World!");

  pinMode(PIN_MOTOR1_A, OUTPUT);
  pinMode(PIN_MOTOR1_B, OUTPUT);
}

void loop()
{
  Serial.println("pinA HIGH, pinB LOW");
  digitalWrite(PIN_MOTOR1_A, HIGH);
  digitalWrite(PIN_MOTOR1_B, LOW);
  delay(1000);
  Serial.println("pinA LOW, pinB HIGH");
  digitalWrite(PIN_MOTOR1_A, LOW);
  digitalWrite(PIN_MOTOR1_B, HIGH);
  delay(500);
}
