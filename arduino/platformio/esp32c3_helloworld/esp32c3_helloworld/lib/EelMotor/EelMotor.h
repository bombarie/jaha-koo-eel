#ifndef ESP32EELMOTOR_h
#define ESP32EELMOTOR_h

#include "Arduino.h"

class EelMotor
{
public:
    EelMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t ledChan);
    EelMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t ledChan, uint8_t resolution);
    EelMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t ledChan, uint8_t resolution, long freq);
    void motorGo(long pwmVal);
    void motorRev(long pwmVal);
    void motorStop();  // Sets both signals to low to allow motor to spin down
    void motorBrake(); // Sets boths signals to high to hard brake the motor

private:
    uint8_t _pinIN1;        // Pin 1 to MX1508
    uint8_t _pinIN2;        // Pin 2 to MX1508
    uint8_t _ledChan;       // ESP32 ledc Channel for PWM
    uint8_t resolution = 8; // PWM Resolution
    long freq = 2500;       // PWM Freq
    long _pwmVal;           // PWM Value (speed)
    long _maxpwm;           // Max PWM Value of the Motor
};

#endif
