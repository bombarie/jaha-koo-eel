#include "EelMotor.h"

EelMotor::EelMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t ledChan)
{
    EelMotor(pinIN1, pinIN2, ledChan, resolution, freq);
}

EelMotor::EelMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t ledChan, uint8_t resolution)
{
    this->resolution = resolution;

    EelMotor(pinIN1, pinIN2, ledChan, resolution, freq);
}

EelMotor::EelMotor(uint8_t pinIN1, uint8_t pinIN2, uint8_t ledChan, uint8_t resolution, long freq)
{
    this->freq = freq;
    this->resolution = resolution;

    _pinIN1 = pinIN1;
    _pinIN2 = pinIN2;
    _ledChan = ledChan; // ESP32 LED Channel for PWM to Pin

    pinMode(_pinIN1, OUTPUT);
    pinMode(_pinIN2, OUTPUT);
    ledcSetup(_ledChan, freq, resolution); // Setup channel at specified Hz with 8 (0-255), 12 (0-4095), or 16 (0-65535) bit resolution

    if (resolution == 8)
    {
        _maxpwm = 255;
    } // Sets a flag on the motor so the object knows the max pwm value
    if (resolution == 12)
    {
        _maxpwm = 4095;
    }
    if (resolution == 16)
    {
        _maxpwm = 65535;
    }

    motorStop();
}

void EelMotor::motorStop()
{
    this->_pwmVal = 0;

    ledcDetachPin(_pinIN1);
    ledcDetachPin(_pinIN2);
    digitalWrite(_pinIN1, LOW);
    digitalWrite(_pinIN2, LOW);
}

void EelMotor::motorBrake()
{
    this->_pwmVal = 0;

    ledcDetachPin(_pinIN1);
    ledcDetachPin(_pinIN2);
    digitalWrite(_pinIN1, HIGH);
    digitalWrite(_pinIN2, HIGH);
}

void EelMotor::motorGo(long pwmSpeed)
{
    this->_pwmVal = pwmSpeed;

    if (reversed)
    {
        _motorRev();
    }
    else
    {
        _motorGo();
    }
}

void EelMotor::motorRev(long pwmSpeed)
{
    this->_pwmVal = pwmSpeed;

    if (reversed)
    {
        _motorGo();
    }
    else
    {
        _motorRev();
    }
}

// Slow-decay drive: hold the non-driven pin HIGH instead of LOW, so the
// DRV8833 brakes between PWM pulses instead of coasting through its body
// diodes. Cuts the flyback ripple/noise that fast decay produces on every
// PWM edge (confirmed on hardware).
void EelMotor::_motorGo()
{
    ledcDetachPin(_pinIN1);
    digitalWrite(_pinIN1, HIGH);

    ledcAttachPin(_pinIN2, _ledChan);
    ledcWrite(_ledChan, _pwmVal);
}

void EelMotor::_motorRev()
{
    ledcDetachPin(_pinIN2);
    digitalWrite(_pinIN2, HIGH);

    ledcAttachPin(_pinIN1, _ledChan);
    ledcWrite(_ledChan, _pwmVal);
}
