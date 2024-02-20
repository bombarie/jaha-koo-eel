#include <Arduino.h>
#include <ESP32MX1508.h>
#include <Adafruit_BusIO_Register.h>

#define PIN_MOTOR1_A D5
#define PIN_MOTOR1_B D4
#define CH1 4 // 16 Channels (0-15) are availible
#define CH2 5 // Make sure each pin is a different channel and not in use by other PWM devices (servos, LED's, etc)

// Optional Parameters
uint8_t _RES = 8;    // Resolution in bits:  8 (0-255),  12 (0-4095), or 16 (0-65535)
long _FREQ = 1000; // PWM Frequency in Hz

MX1508 motorA(PIN_MOTOR1_A, PIN_MOTOR1_B, CH1, CH2, _RES, _FREQ); // Default-  8 bit resoluion at 2500 Hz

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello World!");
}

void loop()
{
  Serial.println("motorA go forward 200");
  motorA.motorGo(200); // Pass the speed to the motor: 0-255 for 8 bit resolution
  delay(1000);
  Serial.println("motorA stop");
  motorA.motorStop(); // Soft Stop    -no argument
  delay(1000);
  Serial.println("motorA go backward 100");
  motorA.motorRev(100); // Pass the speed to the motor: 0-255 for 8 bit resolution
  delay(1000);
  Serial.println("motorA brake");
  motorA.motorBrake(); // Hard Stop    -no arguement
  delay(1000);

  // Serial.println("Hello....");
  // delay(1000);
  // Serial.println("....WUNDERBAR World!");
  // delay(1000);
}
