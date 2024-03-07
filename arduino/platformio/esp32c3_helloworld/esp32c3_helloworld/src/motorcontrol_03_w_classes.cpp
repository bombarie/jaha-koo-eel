#include <Arduino.h>
#include <EelMotor.h>
#include <Wire.h>
// #include <Adafruit_BusIO_Register.h>

#define motor1a_pin D5
#define motor1b_pin D4
#define motor1_chan 4 // 6 Channels (0-5) are availible

// Optional Parameters
uint8_t _RES = 8;  // Resolution in bits:  8 (0-255),  12 (0-4095), or 16 (0-65535)
long _FREQ = 1000; // PWM Frequency in Hz

EelMotor motor1(motor1a_pin, motor1b_pin, motor1_chan, _RES, _FREQ);

void motorGo(byte speed);
void motorRev(byte speed);
void motorStop();
void motorBrake();

void setup()
{
  Serial.begin(115200);

  motor1.reversed = true;

  Serial.println("Hello motorcontrol_02_w_ledc!");
}

void loop()
{
  Serial.println("motorA go forward 200");
  motor1.motorGo(200);
  delay(1000);
  Serial.println("motorA stop");
  motor1.motorStop();
  delay(1000);
  Serial.println("motorA go backward 100");
  motor1.motorRev(100);
  delay(1000);
  Serial.println("motorA brake");
  motor1.motorBrake();
  delay(1000);

}
