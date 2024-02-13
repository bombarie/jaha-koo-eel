#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define PIN 17      // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 2 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// define the methods used in this sketch (prevents compile errors)
void processMIDI(void);
void printBytes(const byte *data, unsigned int size);
uint16_t processDeadband(uint16_t val, uint16_t range);

enum DEADBAND_INPUT_RANGE
{
  _256 = 127,
  _1024 = 1023
};

byte ledPWMVal = 0;
uint16_t audioVal = 0;
byte bodyExpressionVal = 0;
byte aux1_val = 0;
byte aux2_val = 0;
byte aux3_val = 0;

uint16_t neopixelHVal = 0;
byte neopixelSVal = 0;
byte neopixelVVal = 0;

byte neopixel_rVal = 0;
byte neopixel_gVal = 0;
byte neopixel_bVal = 0;

byte bitmash_nood1a = 0;
byte bitmash_nood1b = 0;
byte bitmash_nood2a = 0;
byte bitmash_nood2b = 0;

#define LED_PIN 10
#define PWM_OUT_1 20
#define AUX1_PIN 22
#define AUX2_PIN 23
#define AUX3_PIN 6

void BlinkLed(byte num) // Basic blink function
{
  for (byte i = 0; i < num; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello World");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PWM_OUT_1, OUTPUT);
  pinMode(AUX1_PIN, OUTPUT);
  pinMode(AUX2_PIN, OUTPUT);
  pinMode(AUX3_PIN, OUTPUT);

  BlinkLed(2);

  // from https://www.pjrc.com/teensy/teensy31.html
  analogWriteResolution(10);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

long prevSerialPrintMills;
long serialPrintInterval = 200;

long prevBitmashChangeChannelMills;
long bitmashChangeChannelInterval = 10;
byte bitmashSendChannel = 0;

uint16_t bitmashed_out = 0;
uint16_t bitmashed_outs[] = {0, 0, 0, 0};

float deadbandLowerThreshold = 0.125; // everything lower than this is capped off on the receiving end
float deadbandUpperThreshold = 0.915; // everything higher than this is capped off on the receiving end

void loop()
{
  /*
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  */

  //* TMP -> TRYOUT bitmash a byte
  // byte bitmashed = bitmash_nood1a >> 5;
  // bitmashed = bitmashed << 2;
  // bitmashed += bitmash_nood1b >> 5;
  // bitmashed = bitmashed << 2;
  // bitmashed += bitmash_nood2a >> 5;
  // bitmashed = bitmashed << 2;
  // bitmashed += bitmash_nood2b >> 5;

  if (millis() - prevBitmashChangeChannelMills > bitmashChangeChannelInterval)
  {
    bitmashSendChannel++;
    if (bitmashSendChannel > 3)
    {
      bitmashSendChannel = 0;
    }

    prevBitmashChangeChannelMills = millis();
  }

  switch (bitmashSendChannel)
  {
  case 0:
    bitmashed_out = 0x1 << 9;
    // bitmashed_out += bitmash_nood1a;
    bitmashed_out += (bitmash_nood1a >> 2);
    // bitmashed_out += 1; // this may not be necessary but could help prevent the signal jitter from allowing the higher bits from slipping into a lower bit's position
    break;
  case 1:
    bitmashed_out = 0x1 << 8;
    // bitmashed_out += bitmash_nood1b;
    bitmashed_out += (bitmash_nood1b >> 2);
    // bitmashed_out += 1; // this may not be necessary but could help prevent the signal jitter from allowing the higher bits from slipping into a lower bit's position
    break;
  case 2:
    bitmashed_out = 0x1 << 7;
    // bitmashed_out += bitmash_nood2a;
    bitmashed_out += (bitmash_nood2a >> 2);
    // bitmashed_out += 1; // this may not be necessary but could help prevent the signal jitter from allowing the higher bits from slipping into a lower bit's position
    break;
  case 3:
    bitmashed_out = 0x1 << 6;
    // bitmashed_out += bitmash_nood2b;
    bitmashed_out += (bitmash_nood2b >> 2);
    // bitmashed_out += 1; // this may not be necessary but could help prevent the signal jitter from allowing the higher bits from slipping into a lower bit's position
    break;
  }

  bitmashed_out = processDeadband(bitmashed_out, DEADBAND_INPUT_RANGE::_1024);
  bitmashed_outs[bitmashSendChannel] = bitmashed_out;

  // HARD overwrite for now, just testing the reliability of this approach
  // bitmashed_out = 0x1 << 8;
  // bitmashed_out += (bitmash_nood1b >> 2);
  // bitmashed_out = processDeadband(bitmashed_out, DEADBAND_INPUT_RANGE::_1024);
  // bitmashed_outs[1] = bitmashed_out;

  analogWrite(A14, bitmashed_out);

  if (millis() - prevSerialPrintMills > serialPrintInterval)
  {
    Serial.print("bitmash_nood1a: ");
    Serial.print(bitmash_nood1a);
    Serial.print(", bitmash_nood1b: ");
    Serial.print(bitmash_nood1b);
    Serial.print(", bitmash_nood2a: ");
    Serial.print(bitmash_nood2a);
    Serial.print(", bitmash_nood2b: ");
    Serial.print(bitmash_nood2b);
    Serial.print(" -> bitmashed_outs: ");
    Serial.print(bitmashed_outs[0], BIN);
    Serial.print(", ");
    Serial.print(bitmashed_outs[1], BIN);
    Serial.print(", ");
    Serial.print(bitmashed_outs[2], BIN);
    Serial.print(", ");
    Serial.println(bitmashed_outs[3], BIN);
    prevSerialPrintMills = millis();
  }

  /*/
  analogWrite(A14, audioVal << 2); // ORIGINAL
  //*/

  analogWrite(PWM_OUT_1, bodyExpressionVal);
  analogWrite(AUX1_PIN, aux1_val);
  analogWrite(AUX2_PIN, aux2_val);
  analogWrite(AUX3_PIN, aux3_val);

  // local led as debug indicator
  analogWrite(LED_PIN, ledPWMVal);

  /*
  Serial.println("audioVal: " + String(audioVal));
  Serial.println("bodyExpressionVal: " + String(bodyExpressionVal));
  Serial.println(">aux1_val:" + String(aux1_val));
  Serial.println(">aux2_val:" + String(aux2_val));
  Serial.println(">aux3_val:" + String(aux3_val));
  */

  // usbMIDI.read() needs to be called rapidly from loop().  When
  // each MIDI messages arrives, it return true.  The message must
  // be fully processed before usbMIDI.read() is called again.
  if (usbMIDI.read())
  {
    processMIDI();
  }

  // delay(1000/100);
}

void processMIDI(void)
{
  byte type, channel, data1, data2, cable;

  // fetch the MIDI message, defined by these 5 numbers (except SysEX)
  //
  type = usbMIDI.getType();       // which MIDI message, 128-255
  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
  cable = usbMIDI.getCable();     // which virtual cable with MIDIx8, 0-7

  // uncomment if using multiple virtual cables
  // Serial.print("cable ");
  // Serial.print(cable, DEC);
  // Serial.print(": ");

  // print info about the message
  //
  switch (type)
  {
  case usbMIDI.NoteOff: // 0x80
    Serial.print("Note Off, ch=");
    Serial.print(channel, DEC);
    Serial.print(", note=");
    Serial.print(data1, DEC);
    Serial.print(", velocity=");
    Serial.println(data2, DEC);
    break;

  case usbMIDI.NoteOn: // 0x90
    Serial.print("Note On, ch=");
    Serial.print(channel, DEC);
    Serial.print(", note=");
    Serial.print(data1, DEC);
    Serial.print(", velocity=");
    Serial.println(data2, DEC);
    break;

  case usbMIDI.AfterTouchPoly: // 0xA0
    Serial.print("AfterTouch Change, ch=");
    Serial.print(channel, DEC);
    Serial.print(", note=");
    Serial.print(data1, DEC);
    Serial.print(", velocity=");
    Serial.println(data2, DEC);
    break;

  case usbMIDI.ControlChange: // 0xB0
    // Serial.print("Control Change, ch=");
    // Serial.print(channel, DEC);
    // Serial.print(", control=");
    // Serial.print(data1, DEC);
    // Serial.print(", value=");
    // Serial.println(data2, DEC);

    if (channel == 1 && data1 == 0)
    {
      ledPWMVal = data2 * 2;
      audioVal = processDeadband(data2, 256) * 2;
      if (audioVal > 128)
      {
        audioVal++; // cheap way to make midi max 127 map to analog max 255
      }
    }

    if (channel == 1 && data1 == 1)
    {
      bodyExpressionVal = data2 * 2;
      if (bodyExpressionVal > 128)
      {
        bodyExpressionVal++; // cheap way to make midi max 127 map to analog max 255
      }
    }

    if (channel == 1 && data1 == 2)
    {
      aux1_val = data2 * 2;
      if (aux1_val > 128)
      {
        aux1_val++; // cheap way to make midi max 127 map to analog max 255
      }
    }

    if (channel == 1 && data1 == 3)
    {
      aux2_val = data2 * 2;
      if (aux2_val > 128)
      {
        aux2_val++; // cheap way to make midi max 127 map to analog max 255
      }
    }

    if (channel == 1 && data1 == 4)
    {
      aux3_val = data2 * 2;
      if (aux3_val > 128)
      {
        aux3_val++; // cheap way to make midi max 127 map to analog max 255
      }
    }

    if (channel == 1 && data1 == 10)
    {
      neopixelHVal = (data2 * 2) << 8;
    }
    if (channel == 1 && data1 == 11)
    {
      neopixelSVal = data2 * 2;
    }
    if (channel == 1 && data1 == 12)
    {
      neopixelVVal = data2 * 2;
    }

    if (channel == 1 && data1 == 20)
    {
      neopixel_rVal = data2 * 2;
    }
    if (channel == 1 && data1 == 21)
    {
      neopixel_gVal = data2 * 2;
    }
    if (channel == 1 && data1 == 22)
    {
      neopixel_bVal = data2 * 2;
    }

    if (channel == 1 && data1 == 30)
    {
      bitmash_nood1a = data2;
      // bitmash_nood1a = processDeadband(data2, DEADBAND_INPUT_RANGE::_256);
    }
    if (channel == 1 && data1 == 31)
    {
      bitmash_nood1b = data2;
      // bitmash_nood1b = processDeadband(data2, DEADBAND_INPUT_RANGE::_256);
    }
    if (channel == 1 && data1 == 32)
    {
      bitmash_nood2a = data2;
      // bitmash_nood2a = processDeadband(data2, DEADBAND_INPUT_RANGE::_256);
    }
    if (channel == 1 && data1 == 33)
    {
      bitmash_nood2b = data2;
      // bitmash_nood2b = processDeadband(data2, DEADBAND_INPUT_RANGE::_256);
    }

    break;

  case usbMIDI.ProgramChange: // 0xC0
    Serial.print("Program Change, ch=");
    Serial.print(channel, DEC);
    Serial.print(", program=");
    Serial.println(data1, DEC);
    break;

  case usbMIDI.AfterTouchChannel: // 0xD0
    Serial.print("After Touch, ch=");
    Serial.print(channel, DEC);
    Serial.print(", pressure=");
    Serial.println(data1, DEC);
    break;

  case usbMIDI.PitchBend: // 0xE0
    Serial.print("Pitch Change, ch=");
    Serial.print(channel, DEC);
    Serial.print(", pitch=");
    Serial.println(data1 + data2 * 128, DEC);
    break;

  case usbMIDI.SystemExclusive: // 0xF0
    // Messages larger than usbMIDI's internal buffer are truncated.
    // To receive large messages, you *must* use the 3-input function
    // handler.  See InputFunctionsComplete for details.
    Serial.print("SysEx Message: ");
    printBytes(usbMIDI.getSysExArray(), data1 + data2 * 256);
    Serial.println();
    break;

  case usbMIDI.TimeCodeQuarterFrame: // 0xF1
    Serial.print("TimeCode, index=");
    Serial.print(data1 >> 4, DEC);
    Serial.print(", digit=");
    Serial.println(data1 & 15, DEC);
    break;

  case usbMIDI.SongPosition: // 0xF2
    Serial.print("Song Position, beat=");
    Serial.println(data1 + data2 * 128);
    break;

  case usbMIDI.SongSelect: // 0xF3
    Serial.print("Sond Select, song=");
    Serial.println(data1, DEC);
    break;

  case usbMIDI.TuneRequest: // 0xF6
    Serial.println("Tune Request");
    break;

  case usbMIDI.Clock: // 0xF8
    Serial.println("Clock");
    break;

  case usbMIDI.Start: // 0xFA
    Serial.println("Start");
    break;

  case usbMIDI.Continue: // 0xFB
    Serial.println("Continue");
    break;

  case usbMIDI.Stop: // 0xFC
    Serial.println("Stop");
    break;

  case usbMIDI.ActiveSensing: // 0xFE
    Serial.println("Actvice Sensing");
    break;

  case usbMIDI.SystemReset: // 0xFF
    Serial.println("System Reset");
    break;

  default:
    Serial.println("Opps, an unknown MIDI message type!");
  }
}

uint16_t processDeadband(uint16_t val, uint16_t range)
{
  switch (range)
  {
  case DEADBAND_INPUT_RANGE::_256:
    return constrain(map(val, 0, 127, 15, 116), 0, 127);
    break;
  case DEADBAND_INPUT_RANGE::_1024:
    // return constrain(map(val, 0, 1023, 127, 936), 0, 1023);
    return constrain(map(val, 0, 1023, 130, 936), 0, 1023);
    break;
  }
}

void printBytes(const byte *data, unsigned int size)
{
  while (size > 0)
  {
    byte b = *data++;
    if (b < 16)
      Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1)
      Serial.print(' ');
    size = size - 1;
  }
}
