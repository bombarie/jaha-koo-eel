#include <Arduino.h>
#include <MX1508.h>
// #include <PID_v1.h>
#include "ArduPID.h"

/*
  2023-01-04 -  MEH.... how the fuck does this PID stuff work? I'm using Teleplot to plot the values. I can't seem to get a good working system going.
                The pid controller appears to work fine above ~115rpm. Lower than that the output rpm just doesn't seem to approximate the setpoint. Wha? 
                Does it have something to do with the bias setting? I don't quite get that one....
                Leaving this for now -> it's of later concern
*/

#define MOTOR_PIN_A 5
#define MOTOR_PIN_B 6

// MX1508 schematics(in Chinese) can be found here at: http://sales.dzsc.com/486222.html
/*
 * MX1508(uint8_t pinIN1, uint8_t pinIN2, DecayMode decayMode, NumOfPwmPins numPWM);
 * DecayMode must be FAST_DECAY or SLOW_DECAY,
 * NumOfPwmPins, either use 1 or 2 pwm.
 * I recommend using 2 pwm pins per motor so spinning motor forward and backward gives similar response.
 * if using 1 pwm pin, make sure its pinIN1, then set pinIN2 to any digital pin. I dont recommend this setting because
 * we need to use FAST_DECAY in one direction and SLOW_DECAY for the other direction.
 */
// MX1508 motorA(MOTOR_PIN_A,MOTOR_PIN_B, FAST_DECAY, NUMPWM);
MX1508 motorA(MOTOR_PIN_A, MOTOR_PIN_B);
#define MAX_RPM 130

// define methods
void interruptA();

#define LED_PIN 13

#define SETPOINT_POT_PIN A0
#define Pterm_POT_PIN A1
#define Iterm_POT_PIN A2
#define Dterm_POT_PIN A5

#define MOTOR_ENCODER_A_PIN 2
#define MOTOR_ENCODER_B_PIN 3

// timer stuff
uint32_t prevChangeRPM;
uint8_t rpmType = 0;

// update rpm-print stuff
uint32_t prevPrintRPM;

uint16_t rpmTarget = 0;

volatile float RPM = 0;
volatile float RPM_SMOOTH = 0;
volatile uint32_t lastA = 0;
volatile bool motordir = true;
volatile uint16_t encoderCount = 0;
volatile float revs = 0.0;
volatile uint16_t interruptCount = 0;

// Define Variables we'll be connecting to
double Setpoint, Input, Output;

// Specify the links and initial tuning parameters
double Kp = 2, Ki = 4, Kd = 1;
ArduPID myController;

// These let us convert ticks-to-RPM
#define GEARING 20
#define ENCODERMULT 12

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(MOTOR_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(MOTOR_ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MOTOR_ENCODER_A_PIN), interruptA, RISING);

  myController.begin(&Input, &Output, &Setpoint, Kp, Ki, Kd);
  // myController.reverse()               // Uncomment if controller output is "reversed"
  // myController.setSampleTime(10);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
  myController.setOutputLimits(48, 255);
  myController.setBias(0.0);
  myController.setWindUpLimits(-10, 10); // Groth bounds for the integral term to prevent integral wind-up

  myController.start();
  // myController.reset();               // Used for resetting the I and D terms - only use this if you know what you're doing
  // myController.stop();                // Turn off the PID controller (compute() will not do anything until start() is called)

  prevChangeRPM = millis();
  Setpoint = 0;

  rpmType = 0;
}

uint32_t mills;
uint16_t currRPM = 0;
float revsSmoothed = 0.0;
void loop()
{
  mills = millis();

  int16_t potVal = analogRead(SETPOINT_POT_PIN);

  // 2, 5, 1
  Kp = map(analogRead(Pterm_POT_PIN), 0, 1023, 0, 100) / 10.0;
  Ki = map(analogRead(Iterm_POT_PIN), 0, 1023, 0, 100) / 10.0;
  // Kd = map(analogRead(Dterm_POT_PIN), 0, 1023, 0, 100) / 10.0;
  
  float bias = map(analogRead(Dterm_POT_PIN), 0, 1023, 0, 2550) / 10.0;
  myController.setBias(bias);
  
  myController.setCoefficients(Kp, Ki, Kd);

  rpmTarget = map(potVal, 0, 1023, 0, MAX_RPM);

  Input = currRPM;
  Setpoint = rpmTarget;

  myController.compute();

  potVal = map(potVal, 0, 1023, -255, 255);
  // Serial.println("pot: " + String(analogRead(POT_PIN)));

  // lowpass / smooth
  RPM_SMOOTH = 0.96 * RPM_SMOOTH + 0.04 * RPM;

  Serial.println(">p-term:" + String(Kp));
  Serial.println(">i-term:" + String(Ki));
  Serial.println(">d-term:" + String(Kd));

  Serial.println(">p-val:" + String(myController.P()));
  Serial.println(">i-val:" + String(myController.I()));
  Serial.println(">d-val:" + String(myController.D()));
  Serial.println(">bias:" + String(bias));

  Serial.println(">RPM (raw): " + String(RPM));
  Serial.println(">RPM (smoothed): " + String(RPM_SMOOTH));
  Serial.println(">Setpoint: " + String(Setpoint));
  Serial.println(">Output: " + String(Output));
  Serial.println(">currRPM: " + String(currRPM));
  Serial.println(">Input: " + String(Input));

  motorA.motorGo(Output);


  if (mills - prevPrintRPM > (1000 / 50))
  {
    // Serial.println(">RPM: " + String(RPM));

    revs /= interruptCount;
    revs = 1.0 / revs;   // rev per us
    revs *= 1000000;     // rev per sec
    revs *= 60;          // rev per min
    revs /= GEARING;     // account for gear ratio
    revs /= ENCODERMULT; // account for multiple ticks per rotation

    if (!isnan(revs))
    {
      revsSmoothed = 0.95 * revsSmoothed + 0.05 * revs;
      // currRPM = (int)(0.9 * currRPM + 0.1 * revs);
      currRPM = (int)revsSmoothed;
    }

    Serial.println(">revs: " + String(revs));
    Serial.println(">revsSmoothed: " + String(revsSmoothed));

    interruptCount = 0;
    revs = 0.0;

    prevPrintRPM = mills;
  }

  delay(1000 / 50);
}

void interruptA()
{
  // motordir = digitalRead(MOTOR_ENCODER_B_PIN);

  // digitalWrite(LED_PIN, HIGH);
  uint32_t currA = micros();
  if (lastA < currA)
  {
    // did not wrap around
    float rev = currA - lastA; // us

    // filter out inrealistic values
    if (rev > 1700)
    {
      interruptCount++;
      revs += rev;

      rev = 1.0 / rev;    // rev per us
      rev *= 1000000;     // rev per sec
      rev *= 60;          // rev per min
      rev /= GEARING;     // account for gear ratio
      rev /= ENCODERMULT; // account for multiple ticks per rotation

      // filter out impossible values
      if (rev < 145)
      {
        RPM = rev;
      }
    }
  }
  lastA = currA;
  // digitalWrite(LED_PIN, LOW);
}
