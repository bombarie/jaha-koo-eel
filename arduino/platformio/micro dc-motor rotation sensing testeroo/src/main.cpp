#include <Arduino.h>

// define methods
void updateMotorSpeed();
void interruptA();

#define LED_PIN 13
#define motorPWMOutPin 9

#define MOTOR_ENCODER_A_PIN 2
#define MOTOR_ENCODER_B_PIN 3

// timer stuff
uint32_t prevChangeRPM;
uint8_t rpmType = 0;

// update rpm-print stuff
uint32_t prevPrintRPM;

volatile float RPM = 0;
volatile uint32_t lastA = 0;
volatile bool motordir = true;
volatile uint16_t encoderCount = 0;
volatile float revs = 0.0;
volatile uint16_t interruptCount = 0;

// These let us convert ticks-to-RPM
#define GEARING 20
#define ENCODERMULT 12

void interruptA()
{
  // motordir = digitalRead(MOTOR_ENCODER_B_PIN);

  // digitalWrite(LED_PIN, HIGH);
  uint32_t currA = micros();
  if (lastA < currA)
  {
    // did not wrap around
    float rev = currA - lastA; // us
    
    interruptCount++;
    revs += rev;

    rev = 1.0 / rev;           // rev per us
    rev *= 1000000;            // rev per sec
    rev *= 60;                 // rev per min
    rev /= GEARING;            // account for gear ratio
    rev /= ENCODERMULT;        // account for multiple ticks per rotation
    RPM = rev;
  }
  lastA = currA;
  // digitalWrite(LED_PIN, LOW);
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(motorPWMOutPin, OUTPUT);

  pinMode(MOTOR_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(MOTOR_ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MOTOR_ENCODER_A_PIN), interruptA, RISING);

  prevChangeRPM = millis();

  rpmType = 0;
  updateMotorSpeed();
}

uint32_t mills;
void loop()
{
  mills = millis();
  // lowpass / smooth
  // RPM = 0.96 * RPM + 0.04 * RPM;

  if (mills - prevPrintRPM > (1000/30))
  {
    Serial.println(">RPM: " + String(RPM));

    revs /= interruptCount;
    revs = 1.0 / revs;           // rev per us
    revs *= 1000000;            // rev per sec
    revs *= 60;                 // rev per min
    revs /= GEARING;            // account for gear ratio
    revs /= ENCODERMULT;        // account for multiple ticks per rotation

    Serial.println(">revs: " + String(revs));

    interruptCount = 0;
    revs = 0.0;
    
    prevPrintRPM = mills;
  } 

  if (mills - prevChangeRPM > 2500)
  {
    rpmType++;
    rpmType = rpmType % 3;

    updateMotorSpeed();

    // Serial.println("changed rpm to " + String(rpmType));

    prevChangeRPM = mills;
  }

  delay(1000/50);
}

void updateMotorSpeed()
{
  switch (rpmType)
  {
  case 0:
    analogWrite(motorPWMOutPin, 160);
    break;
  case 1:
    analogWrite(motorPWMOutPin, 200);
    break;
  case 2:
    analogWrite(motorPWMOutPin, 240);
    break;
  }
}