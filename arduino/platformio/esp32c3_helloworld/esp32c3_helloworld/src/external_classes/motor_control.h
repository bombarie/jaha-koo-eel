#include <EelMotor.h>

#define motor1a_pin D5
#define motor1b_pin D4
#define motor2a_pin D3
#define motor2b_pin D2

#define motor1_chan 4 // 6 Channels (ESP32-C3) (0-5) are availible
#define motor2_chan 5 // 6 Channels (ESP32-C3) (0-5) are availible

EelMotor motor1(motor1a_pin, motor1b_pin, motor1_chan, resolution, freq);
EelMotor motor2(motor2a_pin, motor2b_pin, motor2_chan, resolution, freq);

int16_t motor1Val;
int16_t motor2Val;

void initMotors();
void calcMotorValues();
void driveMotors();

void initMotors()
{
  motor1.reversed = true;
  motor2.reversed = true;
}

uint8_t motorStrength;

void calcMotorValues()
{
  byte motorPowerRange = 255;
  byte motorOutMode = 0;

  //* position of AUX4 selects diff motor power ranges
  if (data.ch[TX_AUX4] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    motorOutMode = 0; // linear
    motorPowerRange = 255;
  }
  else if (data.ch[TX_AUX4] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    motorOutMode = 0; // linear
    motorPowerRange = 200;
  }
  else
  {
    motorOutMode = 1; // wiggle
    motorPowerRange = 255;
  }
  //*/

  int16_t mix = 1000;

  /* position of AUX4 selects diff motor output modes
  if (data.ch[TX_AUX4] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    motorOutMode = 0;
  }
  else if (data.ch[TX_AUX4] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    motorOutMode = 1;
  }
  else
  {
    motorOutMode = 2;
  }
  //*/

  // calc drive strength and determine fwd/rev direction
  uint16_t pitchOffset = abs(data.ch[TX_PITCH] - SBUS_VAL_CENTER);
  uint16_t rollOffset = abs(data.ch[TX_ROLL] - SBUS_VAL_CENTER);
  double stickDistFromCenter = sqrt(pow(pitchOffset, 2) + pow(rollOffset, 2));
  int16_t motorStrength = constrain(map(stickDistFromCenter, 0, 800, 0, motorPowerRange), 0, motorPowerRange);
  if (pitchOffset > SBUS_VAL_DEADBAND)
  {
    if ((data.ch[TX_PITCH] - SBUS_VAL_CENTER) < 0)
    {
      motorStrength = -motorStrength;
    }
  }

  if (motorOutMode == 0)
  {

    //*/
    // motor1Val = constrain(map(data.ch[TX_PITCH], SBUS_VAL_MIN, SBUS_VAL_MAX, -motorPowerRange, motorPowerRange), -255, 255);
    // motor2Val = motor1Val;

    int16_t throttleVal = motorStrength;
    EVERY_N_MILLIS(250)
    {
      // Serial.print("motorPowerRange: " + String(motorPowerRange));
      // Serial.println(", throttleVal: " + String(motor2Val));
    }

    // float mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 750, 250) / 1000.0; // gives a range of .25-.75

    // /*
    // range 0.0-1.0, then an exponent, then map to 250-750
    mix = constrain(map(data.ch[TX_ROLL], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 2000), 0, 2000); // gives a range of 0-2000
    // mix = pow(mix, 1.4);
    // mix = map((mix * 1000.0), 1000, 0, 250, 750) / 1000.0; // reverse the input range because we want to reverse the steering
    // mix = map((mix * 1000.0), 1000, 0, 0, 1000) / 1000.0; // reverse the input range because we want to reverse the steering
    //*/

    // shitty hack
    uint16_t mixLeft = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER, SBUS_VAL_MIN, 1000, 0), 0, 1000);
    uint16_t mixRight = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER, SBUS_VAL_MAX, 1000, 0), 0, 1000);

    // motor1Val = (int16_t)((throttleVal * (2000 - mix)) / mulVal);
    // motor2Val = (int16_t)((throttleVal * mix) / mulVal);
    motor1Val = (int16_t)((throttleVal * mixRight) / 1000);
    motor2Val = (int16_t)((throttleVal * mixLeft) / 1000);

    // taper off motor strength as pitch stick gets closer to center
    if (pitchOffset < 75)
    {
      motor1Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor1Val /= 1000;

      motor2Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor2Val /= 1000;
    }
  }

  // wiggle mode: left/right stick deflection yields amplified motor diff strength.
  if (motorOutMode == 1)
  {
    int16_t throttleVal = motorStrength;

    // shitty hack
    int16_t mixLeft = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER, SBUS_VAL_MIN, 1000, -1000), -1000, 1000);
    int16_t mixRight = constrain(map(data.ch[TX_ROLL], SBUS_VAL_CENTER, SBUS_VAL_MAX, 1000, -1000), -1000, 1000);

    motor1Val = (int16_t)((throttleVal * mixRight) / 1000);
    motor2Val = (int16_t)((throttleVal * mixLeft) / 1000);

    // taper off motor strength as pitch stick gets closer to center
    if (pitchOffset < 75)
    {
      motor1Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor1Val /= 1000;

      motor2Val *= map(pitchOffset, 0, 75, 350, 1000);
      motor2Val /= 1000;
    }
  }

  /*
  doSineMovement = (data.ch[TX_AUX4] < SBUS_SWITCH_MIN_THRESHOLD) ? true : false;
  doSineMovement = false; // override

  // only applies if doSineMovement is true
  // sinCounterIncrement = map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 200, 1000) / 5000.0;
  sinCounterIncrement = 550 / 5000.0; // override for testing
  float sinMulFactor = .9;
  sinMulFactor = constrain(map(data.ch[TX_AUX3], SBUS_VAL_MIN, SBUS_VAL_MAX, 450, 900), 450, 900) / 1000.0;
  float sinMult1 = sin(sinCounter) * sinMulFactor + (1.0 - sinMulFactor);
  float sinMult2 = sin(sinCounter + PI) * sinMulFactor + (1.0 - sinMulFactor);
  sinCounter += sinCounterIncrement;
  // Serial.println("sinCounterIncrement: " + String(sinCounterIncrement));

  if (doSineMovement)
  {
    float ampMul = constrain(map(data.ch[TX_AUX4], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1000), 0, 1000) / 1000.0;
    motor1Val *= (sinMult1 * ampMul) + (1.0 - ampMul);
    motor2Val *= (sinMult2 * ampMul) + (1.0 - ampMul);
  }
  //*/

  motor1Val = constrain(motor1Val, -255, 255);
  motor2Val = constrain(motor2Val, -255, 255);

  // EVERY_N_MILLIS(150)
  // {
  //   Serial.println("pitch: " + String(data.ch[TX_PITCH]) + ", roll: " + String(data.ch[TX_ROLL]) + ", pitchOffset: " + String(pitchOffset) + ", stickDistFromCenter: " + String(stickDistFromCenter) + ", motorStrength: " + String(motorStrength) + ", mix: " + String(mix) + ", motor1Val: " + String(motor1Val) + ", motor2Val: " + String(motor2Val));
  // }
}

void driveMotors()
{
  // MOTOR 1
  if (abs(motor1Val) < SBUS_VAL_DEADBAND)
  {
    motor1.motorStop(); // Soft Stop    -no argument
  }
  if (motor1Val < -SBUS_VAL_DEADBAND)
  {
    motor1.motorRev(-motor1Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor1Val > SBUS_VAL_DEADBAND)
  {
    motor1.motorGo(motor1Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }

  // MOTOR 2
  if (abs(motor2Val) < SBUS_VAL_DEADBAND)
  {
    motor2.motorStop(); // Soft Stop    -no argument
  }
  if (motor2Val < -SBUS_VAL_DEADBAND)
  {
    motor2.motorRev(-motor2Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
  if (motor2Val > SBUS_VAL_DEADBAND)
  {
    motor2.motorGo(motor2Val); // Pass the speed to the motor: 0-255 for 8 bit resolution
  }
}