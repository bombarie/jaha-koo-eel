#include <Arduino.h>
#include <ESP32MX1508.h>
#include <Adafruit_BusIO_Register.h>

#define motor1a_pin D5
#define motor1b_pin D4
#define motor1_chan 4 // 6 Channels (0-5) are availible

// Optional Parameters
uint8_t _RES = 8;  // Resolution in bits:  8 (0-255),  12 (0-4095), or 16 (0-65535)
long _FREQ = 1000; // PWM Frequency in Hz

void motorGo(byte speed);
void motorRev(byte speed);
void motorStop();
void motorBrake();

void setup()
{
  Serial.begin(115200);

  pinMode(motor1a_pin, OUTPUT);
  pinMode(motor1b_pin, OUTPUT);

  // setup ledc channel for motor
  ledcSetup(motor1_chan, _FREQ, _RES);

  Serial.println("Hello motorcontrol_02_w_ledc!");
}

void loop()
{
  Serial.println("motorA go forward 200");
  motorGo(200);
  delay(1000);
  Serial.println("motorA stop");
  motorStop();
  delay(1000);
  Serial.println("motorA go backward 100");
  motorRev(100);
  delay(1000);
  Serial.println("motorA brake");
  motorBrake();
  delay(1000);

  // Serial.println("Hello....");
  // delay(1000);
  // Serial.println("....WUNDERBAR World!");
  // delay(1000);
}

void motorGo(byte speed)
{
  ledcDetachPin(motor1b_pin);
  digitalWrite(motor1b_pin, 0);

  ledcAttachPin(motor1a_pin, motor1_chan);
  ledcWrite(motor1_chan, speed);
}
void motorRev(byte speed)
{
  ledcDetachPin(motor1a_pin);
  digitalWrite(motor1a_pin, 0);

  ledcAttachPin(motor1b_pin, motor1_chan);
  ledcWrite(motor1_chan, speed);
}
void motorStop()
{
  ledcDetachPin(motor1a_pin);
  ledcDetachPin(motor1b_pin);
  digitalWrite(motor1a_pin, 0);
  digitalWrite(motor1b_pin, 0);
}
void motorBrake()
{
  ledcDetachPin(motor1a_pin);
  ledcDetachPin(motor1b_pin);
  digitalWrite(motor1a_pin, 255);
  digitalWrite(motor1b_pin, 255);
}
