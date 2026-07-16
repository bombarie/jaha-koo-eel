/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This sketch is enumerated as USB MIDI device.
 * Following library is required
 * - MIDI Library by Forty Seven Effects
 *   https://github.com/FortySevenEffects/arduino_midi_library
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

uint8_t dac1_pin = A1;
uint8_t dac2_pin = A0;
uint8_t pwm1_pin = 0;
uint8_t pwm2_pin = 1;
uint8_t pwm3_pin = 4;

uint16_t dac1_out = 0;
uint16_t dac2_out = 0;
uint8_t pwm1_out = 0;
uint8_t pwm2_out = 0;
uint8_t pwm3_out = 0;

void handleControlChange(byte channel, byte data1, byte data2);
void writeValuesToOutputs();

void setup()
{
    // Manual begin() is required on core without built-in support e.g. mbed rp2040
    if (!TinyUSBDevice.isInitialized())
    {
        TinyUSBDevice.begin(0);
    }

    Serial.begin(115200);

    usb_midi.setStringDescriptor("TinyUSB MIDI");

    // Initialize MIDI, and listen to all MIDI channels
    // This will also call usb_midi's begin()
    MIDI.begin(MIDI_CHANNEL_OMNI);

    // If already enumerated, additional class driver begin() e.g msc, hid, midi won't take effect until re-enumeration
    if (TinyUSBDevice.mounted())
    {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
    }

    // Attach the handleControlChange function to the MIDI Library. It will
    // be called whenever the Bluefruit receives MIDI Control Change messages.
    MIDI.setHandleControlChange(handleControlChange);

    analogWriteResolution(12);
    analogWrite(dac1_pin, 0);
    analogWrite(dac2_pin, 0);
    pinMode(pwm1_pin, OUTPUT);
    pinMode(pwm2_pin, OUTPUT);
    pinMode(pwm3_pin, OUTPUT);
}

long prevOutputMillis = 0;
long outputInterval = 1000 / 100; // in milliseconds

void loop()
{
#ifdef TINYUSB_NEED_POLLING_TASK
    // Manual call tud_task since it isn't called by Core's background
    TinyUSBDevice.task();
#endif

    // not enumerated()/mounted() yet: nothing to do
    if (!TinyUSBDevice.mounted())
    {
        return;
    }

    // read any new MIDI messages
    MIDI.read();

    if (millis() - prevOutputMillis > outputInterval)
    {
        writeValuesToOutputs();

        prevOutputMillis = millis();
    }
}

void writeValuesToOutputs()
{
    // write to native DAC
    analogWriteResolution(12);
    analogWrite(dac1_pin, dac1_out);
    analogWrite(dac2_pin, dac2_out);
    
    // write to PWM outputs
    // analogWriteResolution(8); // doesn't appear to be necessary
    analogWrite(pwm1_pin, pwm1_out);
    analogWrite(pwm2_pin, pwm2_out);
    analogWrite(pwm3_pin, pwm3_out);
}

void handleControlChange(byte channel, byte data1, byte data2)
{
    Serial.println("Receive CC >>  channel: " + String(channel) + ", data1: " + String(data1) + ", data2: " + String(data2));

    if (channel == 1 && data1 == 0)
    {
        dac1_out = map(data2, 0, 127, 0, 4095);
        // if (audioVal > 128)
        // {
        //     audioVal++; // cheap way to make midi max 127 map to analog max 255
        // }
    }

    if (channel == 1 && data1 == 1)
    {
        dac2_out = map(data2, 0, 127, 0, 4095);
        // bodyExpressionVal = data2 * 2;
        // if (bodyExpressionVal > 128)
        // {
        //     bodyExpressionVal++; // cheap way to make midi max 127 map to analog max 255
        //     }
    }

    if (channel == 1 && data1 == 2)
    {
        pwm1_out = data2 * 2;
        if (pwm1_out > 128)
        {
            pwm1_out++; // cheap way to make midi max 127 map to analog max 255
        }
    }

    if (channel == 1 && data1 == 3)
    {
        pwm2_out = data2 * 2;
        if (pwm2_out > 128)
        {
            pwm2_out++; // cheap way to make midi max 127 map to analog max 255
        }
    }

    if (channel == 1 && data1 == 4)
    {
        pwm3_out = data2 * 2;
        if (pwm3_out > 128)
        {
            pwm3_out++; // cheap way to make midi max 127 map to analog max 255
        }
    }
    /*

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

// n00d individually addressing
if (channel == 1 && data1 == 30)
{
bitmash_nood1a = data2;
}
if (channel == 1 && data1 == 31)
{
bitmash_nood1b = data2;
}
if (channel == 1 && data1 == 32)
{
bitmash_nood2a = data2;
}
if (channel == 1 && data1 == 33)
{
bitmash_nood2b = data2;
}
*/
}
