#include <Arduino.h>
#include <ESP32MX1508.h>
#include <Adafruit_BusIO_Register.h>

#define PINA D5
#define PINB D4
#define CH1 0 // 16 Channels (0-15) are availible
#define CH2 1 // Make sure each pin is a different channel and not in use by other PWM devices (servos, LED's, etc)

// Optional Parameters
#define RES 8     // Resolution in bits:  8 (0-255),  12 (0-4095), or 16 (0-65535)
#define FREQ 5000 // PWM Frequency in Hz

MX1508 motorA(PIN_MOTOR1_A, PIN_MOTOR1_B, CH1, CH2); // Default-  8 bit resoluion at 2500 Hz

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello World!");
}

void loop()
{
  Serial.println("motorA go forward 8");
  motorA.motorGo(8); // Pass the speed to the motor: 0-255 for 8 bit resolution
  delay(2000);
  Serial.println("motorA stop");
  motorA.motorStop(); // Soft Stop    -no argument
  delay(2000);
  Serial.println("motorA go backward 8");
  motorA.motorRev(8); // Pass the speed to the motor: 0-255 for 8 bit resolution
  delay(2000);
  Serial.println("motorA brake");
  motorA.motorBrake(); // Hard Stop    -no arguement
  delay(2000);

  // Serial.println("Hello....");
  // delay(1000);
  // Serial.println("....WUNDERBAR World!");
  // delay(1000);
}
