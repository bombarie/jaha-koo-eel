#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <FlashStorage_SAMD.h>

/*

FYI: Serial Monitor commands:
1 -> outputting only nood1a
2 -> outputting only nood1b
3 -> outputting only nood2a
4 -> outputting only nood2b
a -> outputting all noods (DEFAULT)
s -> toggle serial print values

[ -> lower the bottom end of the deadband range for 4096 mapping
] -> raise the bottom end of the deadband range for 4096 mapping
{ -> lower the top end of the deadband range for 4096 mapping
} -> raise the top end of the deadband range for 4096 mapping

*/

enum DEADBAND_INPUT_RANGE
{
    _256 = 127,
    _1024 = 1023,
    _4096 = 4095
};

struct DeadbandMinMax
{
    uint16_t min;
    uint16_t max;
};

// I manually explored when the values at the receiver start moving. The values are capped off at the lower and upper end of the range.
// So, for both a 256 and 1024 range, I found the values where the receiver starts to move and where it stops moving.
DeadbandMinMax deadbandMinMax256 = {10, 117};
DeadbandMinMax deadbandMinMax1024 = {126, 940}; // FYI: for eel #1, a solid range was {130, 936}
// DeadbandMinMax deadbandMinMax4096 = {134, 3904};
// DeadbandMinMax deadbandMinMax4096 = {128, 3708};
// DeadbandMinMax deadbandMinMax4096 = {128, 3800}; // lower max becomes higher rx values
// 4096 band, the 'ideal' bounds appear to be [~128, ~3772]
// DeadbandMinMax deadbandMinMax4096 = {50, 3975}; // higher max becomes higher rx values
DeadbandMinMax deadbandMinMax4096 = {600, 3400}; // default; may be overwritten by the saved FlashStorage values

// Persists deadbandMinMax4096 across power cycles using flash-emulated storage (internal flash on the SAMD51,
// not classic AVR PROGMEM, which is read-only at runtime). The signature marks whether flash actually holds a
// previously-saved value, since erased/unwritten flash won't reliably read back as this struct's zero value.
struct DeadbandMinMax4096Flash
{
    uint16_t min;
    uint16_t max;
    uint32_t signature;
};
#define DEADBAND_MIN_MAX_4096_FLASH_SIGNATURE 0x5A5AA5A5
FlashStorage(deadbandMinMax4096FlashStore, DeadbandMinMax4096Flash);

bool deadbandMinMax4096Dirty = false;
unsigned long deadbandMinMax4096LastChangeMillis = 0;
const unsigned long deadbandMinMax4096SaveDelay = 5000; // wait for this many quiet ms before committing to flash, to avoid wearing it out

// {512, 640, 768, 896};
// 2026-07-23 -> 473 / 3730
// 2026-08-03 -> 256 / 3755

byte ledPWMVal = 0;
uint16_t audioVal = 0;
byte bodyExpressionVal = 0;
byte aux1_val = 255; // default high
byte aux2_val = 0;
byte aux3_val = 0;
byte cc10_val = 0;     // debug var
uint16_t cc11_val = 0; // debug var

byte bitmash_nood1a = 0;
byte bitmash_nood1b = 0;
byte bitmash_nood2a = 0;
byte bitmash_nood2b = 0;

bool bSerialPrintPWMValues = false;
bool bSerialPrintValues = false;

uint8_t dac1_pin = A1;
uint8_t dac2_pin = A0;
uint8_t pwm1_pin = 4;
uint8_t pwm2_pin = 0;
uint8_t pwm3_pin = 1;

uint16_t dac1_out = 0;
uint16_t dac2_out = 0;
uint8_t pwm1_out = 0;
uint8_t pwm2_out = 0;
uint8_t pwm3_out = 0;

#define NOOD_DATA_PIN dac1_pin

byte channelToPrint = 255;

unsigned long _millis = 0;
unsigned long prevSerialPrintMills;
unsigned long serialPrintInterval = 1000 / 10; // in Hz

unsigned long prevBitmashChangeChannelMills;
unsigned long bitmashChangeChannelInterval = 1000 / 60; // in Hz -> ie 100Hz means that each channel is outputted at 25Hz
byte bitmashSendChannel = 0;

// intermittently, right after the 4th regular channel (index 3), we inject three extra 'channels' that
// send the lower, upper, then lower again bound calibration values (LOW -> HIGH -> LOW), so the receiver
// can calibrate against the actual analog range instead of assuming it lines up exactly with the nominal
// 0-1023 range. The repeated LOW gives the receiver a second, clearer look at the lower bound.
#define NOOD_SEND_CALIB_LOWER 4
#define NOOD_SEND_CALIB_UPPER 5
#define NOOD_SEND_CALIB_LOWER2 6
#define NOOD_CALIB_LOWER_BOUND_VALUE 0    // matches the receiver's "top 3 bits == 0" lower-bound marker check
#define NOOD_CALIB_UPPER_BOUND_VALUE 1023 // matches the receiver's "> 512" upper-bound marker check
#define NOOD_CALIB_TRIGGER_NOTE 72        // MIDI note number for C4 (middle C)

unsigned long noodCalibSequenceIntervalSeconds = 120;                                // how often, in seconds, to inject the calibration sequence
unsigned long noodCalibSequenceInterval = noodCalibSequenceIntervalSeconds * 1000UL; // in ms
unsigned long prevNoodCalibSequenceMillis = 0;

// flip this off to disable the automatic, periodic calibration sequence entirely
bool bEnableNoodCalibSequence = false;

// set by triggerNoodCalibSequence() to request a single calibration sequence be run
// at the next opportune moment, regardless of bEnableNoodCalibSequence / the periodic timer
bool bNoodCalibSequenceRequested = false;

uint16_t bitmashed_out = 0;
uint16_t bitmashed_outs[] = {0, 0, 0, 0};

// this is the value amount that we subtract from 127, to allow some deadband between the nood values.
// Effectively, determines the resolution of the noods. 0 is no deadband, 127 is max deadband.
#define NOOD_VALUES_TRANSMISSION_BANDWIDTH 24

uint16_t n00dSegmentIdentifiers[] = {512, 640, 768, 896}; // corresponds to upper bits 100, 101, 110, 111 (10-bit values)
// uint16_t n00dSegmentIdentifiers[] = {2048, 2560, 3072, 3584};          // corresponds to upper bits 100, 101, 110, 111 (12-bit values)
byte n00dSegmentMaxValue = (127 - NOOD_VALUES_TRANSMISSION_BANDWIDTH); // helps prevent overflows on the upper side

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// methods
void writeValuesToOutputs();
void overrideNoodOutputValues();
void BlinkLed(byte num);
void updateSelectedNoodSendIndex();
void calcNoodOutputValues();
void checkIncomingSerial();
void updateSerialPrintValues();
void serialPrintDebugValues();
void processMIDI(void);
void printBytes(const byte *data, unsigned int size);
void handleControlChange(byte channel, byte data1, byte data2);
void handleNoteOn(byte channel, byte pitch, byte velocity);
void updateProcessLoop();
void updateSendLoop();
void updateCalibrationLed();
void triggerNoodCalibSequence();
uint8_t mapToActualMinMax_256(uint8_t val, uint8_t range);
uint16_t mapToActualMinMax_1024(uint16_t val, uint16_t range);
uint16_t mapToActualMinMax_4096(uint16_t val, uint16_t range);
void loadDeadbandMinMax4096FromFlash();
void markDeadbandMinMax4096Changed();
void updateDeadbandMinMax4096FlashSave();

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
    MIDI.setHandleNoteOn(handleNoteOn);

    pinMode(LED_BUILTIN, OUTPUT);

    analogWriteResolution(12);
    analogWrite(NOOD_DATA_PIN, 0);
    // analogWrite(dac1_pin, 0);
    analogWrite(dac2_pin, 0);
    pinMode(pwm1_pin, OUTPUT);
    pinMode(pwm2_pin, OUTPUT);
    pinMode(pwm3_pin, OUTPUT);

    BlinkLed(2);

    loadDeadbandMinMax4096FromFlash();

    Serial.println("Press '1', '2', '3', '4' to select the channel to print.");
    Serial.println("Press 'a' to print all channels.");
    Serial.println("Press 's' to toggle printing serial values");
}

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

    _millis = millis();

    // handles cycling through the n00d channels at fixed intervals
    updateSelectedNoodSendIndex();

    // the built-in LED (bright red on the Feather M4 Express) lights up while calibration values are being sent
    updateCalibrationLed();

    updateProcessLoop();

    updateSendLoop();

    updateDeadbandMinMax4096FlashSave();
}

void updateProcessLoop()
{
    checkIncomingSerial();

    calcNoodOutputValues();

    // DEBUG -> hard overwrite -> use to test the reliability of the approach
    if (channelToPrint != 255)
    {
        overrideNoodOutputValues();
    }

    if (bSerialPrintValues)
    {
        updateSerialPrintValues();
    }
}

void updateSendLoop()
{
    writeValuesToOutputs();
}

void updateSelectedNoodSendIndex()
{
    // calibration states get shown twice as long so the receiver reliably picks up the bound values
    unsigned long currentChangeChannelInterval = (bitmashSendChannel == NOOD_SEND_CALIB_LOWER || bitmashSendChannel == NOOD_SEND_CALIB_UPPER || bitmashSendChannel == NOOD_SEND_CALIB_LOWER2)
                                                     ? bitmashChangeChannelInterval * 2
                                                     : bitmashChangeChannelInterval;

    if (millis() - prevBitmashChangeChannelMills > currentChangeChannelInterval)
    {
        if (bitmashSendChannel == NOOD_SEND_CALIB_LOWER)
        {
            bitmashSendChannel = NOOD_SEND_CALIB_UPPER;
        }
        else if (bitmashSendChannel == NOOD_SEND_CALIB_UPPER)
        {
            bitmashSendChannel = NOOD_SEND_CALIB_LOWER2;
        }
        else if (bitmashSendChannel == NOOD_SEND_CALIB_LOWER2)
        {
            bitmashSendChannel = 0;
        }
        else
        {
            bitmashSendChannel++;
            if (bitmashSendChannel > 3)
            {
                bool bAutoCalibDue = bEnableNoodCalibSequence && (millis() - prevNoodCalibSequenceMillis > noodCalibSequenceInterval);

                if (bAutoCalibDue || bNoodCalibSequenceRequested)
                {
                    if (bSerialPrintValues)
                    {
                        Serial.println("sending NOOD_SEND_CALIB_LOWER");
                    }
                    // only at the end of a regular sequence (index 3), inject the calibration sequence before wrapping back to 0
                    bitmashSendChannel = NOOD_SEND_CALIB_LOWER;
                    prevNoodCalibSequenceMillis = millis();
                    bNoodCalibSequenceRequested = false;
                }
                else
                {
                    bitmashSendChannel = 0;
                }
            }
        }

        prevBitmashChangeChannelMills = millis();
    }
}

void triggerNoodCalibSequence()
{
    // the actual injection happens inside updateSelectedNoodSendIndex(), once the regular
    // channel cycle reaches its end point (right before wrapping back to channel 0)
    bNoodCalibSequenceRequested = true;
}

void updateCalibrationLed()
{
    bool bCalibrating = (bitmashSendChannel == NOOD_SEND_CALIB_LOWER || bitmashSendChannel == NOOD_SEND_CALIB_UPPER || bitmashSendChannel == NOOD_SEND_CALIB_LOWER2);
    digitalWrite(LED_BUILTIN, bCalibrating ? HIGH : LOW);
}

void calcNoodOutputValues()
{
    switch (bitmashSendChannel)
    {
    case 0:
        bitmashed_out = n00dSegmentIdentifiers[0];
        bitmashed_out += map(bitmash_nood1a, 0, 127, 0, n00dSegmentMaxValue);
        break;
    case 1:
        bitmashed_out = n00dSegmentIdentifiers[1];
        bitmashed_out += map(bitmash_nood1b, 0, 127, 0, n00dSegmentMaxValue);
        break;
    case 2:
        bitmashed_out = n00dSegmentIdentifiers[2];
        bitmashed_out += map(bitmash_nood2a, 0, 127, 0, n00dSegmentMaxValue);
        break;
    case 3:
        bitmashed_out = n00dSegmentIdentifiers[3];
        bitmashed_out += map(bitmash_nood2b, 0, 127, 0, n00dSegmentMaxValue);
        break;
    case NOOD_SEND_CALIB_LOWER:
    case NOOD_SEND_CALIB_LOWER2:
        // intermittently sent so the receiver can calibrate its observed lower bound
        bitmashed_out = NOOD_CALIB_LOWER_BOUND_VALUE;
        return;
    case NOOD_SEND_CALIB_UPPER:
        // intermittently sent so the receiver can calibrate its observed upper bound
        bitmashed_out = NOOD_CALIB_UPPER_BOUND_VALUE;
        return;
    }

    // bitmashed_out = mapToActualMinMax_1024(bitmashed_out, DEADBAND_INPUT_RANGE::_1024);
    // bitmashed_out = mapToActualMinMax_4096(bitmashed_out, DEADBAND_INPUT_RANGE::_4096);
    bitmashed_outs[bitmashSendChannel] = bitmashed_out;
}

void overrideNoodOutputValues()
{
    switch (channelToPrint)
    {
    case 0:
        bitmashed_out = n00dSegmentIdentifiers[0];
        bitmashed_out += map(bitmash_nood1a, 0, 127, 0, n00dSegmentMaxValue);
        // bitmashed_out = mapToActualMinMax_4096(bitmashed_out, DEADBAND_INPUT_RANGE::_4096);
        bitmashed_outs[0] = bitmashed_out;
        break;
    case 1:
        bitmashed_out = n00dSegmentIdentifiers[1];
        bitmashed_out += map(bitmash_nood1b, 0, 127, 0, n00dSegmentMaxValue);
        // bitmashed_out = mapToActualMinMax_4096(bitmashed_out, DEADBAND_INPUT_RANGE::_4096);
        bitmashed_outs[1] = bitmashed_out;
        break;
    case 2:
        bitmashed_out = n00dSegmentIdentifiers[2];
        bitmashed_out += map(bitmash_nood2a, 0, 127, 0, n00dSegmentMaxValue);
        // bitmashed_out = mapToActualMinMax_4096(bitmashed_out, DEADBAND_INPUT_RANGE::_4096);
        bitmashed_outs[2] = bitmashed_out;
        break;
    case 3:
        bitmashed_out = n00dSegmentIdentifiers[3];
        bitmashed_out += map(bitmash_nood2b, 0, 127, 0, n00dSegmentMaxValue);
        // bitmashed_out = mapToActualMinMax_4096(bitmashed_out, DEADBAND_INPUT_RANGE::_4096);
        bitmashed_outs[3] = bitmashed_out;
        break;
    case 10:
        bitmashed_out = map(cc10_val, 0, 127, 0, 1023);
        break;
    case 11:
        bitmashed_out = cc11_val;
        break;
    }
}

void writeValuesToOutputs()
{
    // write to native DAC
    // analogWrite(NOOD_DATA_PIN, mapToActualMinMax_1024(bitmashed_out, DEADBAND_INPUT_RANGE::_1024)); // n00ds -> 10-bit DAC

    // to 12-bit output
    // analogWriteResolution(12);
    bitmashed_out = map(bitmashed_out, 0, 1023, 0, 4095);
    // bitmashed_out += random(-5, 5);
    bitmashed_out = mapToActualMinMax_4096(bitmashed_out, DEADBAND_INPUT_RANGE::_4096);
    analogWrite(NOOD_DATA_PIN, bitmashed_out); // n00ds -> 12-bit DAC

    analogWrite(dac2_pin, audioVal); // audio

    // write to PWM outputs
    // analogWriteResolution(8);
    analogWrite(pwm1_pin, aux1_val);
    analogWrite(pwm2_pin, aux2_val);
    analogWrite(pwm3_pin, aux3_val);
}

void checkIncomingSerial()
{
    if (Serial.available() > 0)
    {
        char inChar = Serial.read();
        switch (inChar)
        {

        case '1':
            Serial.println("Outputting only nood1a");
            channelToPrint = 0;
            break;
        case '2':
            Serial.println("Outputting only nood1b");
            channelToPrint = 1;
            break;
        case '3':
            Serial.println("Outputting only nood2a");
            channelToPrint = 2;
            break;
        case '4':
            Serial.println("Outputting only nood2b");
            channelToPrint = 3;
            break;
        case 'a':
            Serial.println("Outputting all noods");
            channelToPrint = 255;
            break;

        case 'c':
            Serial.println("Trigger calibration sequence");
            triggerNoodCalibSequence();
            break;

        case 'd':
            Serial.println("DEBUG - output midi cc 10 to dac1");
            channelToPrint = 10;
            break;
        case 'e':
            Serial.println("DEBUG - output midi cc 11 to dac1");
            channelToPrint = 11;
            break;

        case 'p':
            bSerialPrintPWMValues = !bSerialPrintPWMValues;
            Serial.println("DEBUG - toggle showing pwm values >> set to " + String(bSerialPrintPWMValues));
            break;

        case 's':
            bSerialPrintValues = !bSerialPrintValues;
            break;

        case 'h':
            Serial.println("Press '1', '2', '3', '4' to select the channel to print.");
            Serial.println("Press '9' to send lower bound (value 0)");
            Serial.println("Press '0' to send upper bound (value 1023)");
            Serial.println("Press 'a' to print all channels.");
            Serial.println("Press 's' to toggle printing serial values");
            Serial.println("Press 'd' to output midi cc 10 to dac1");
            Serial.println("Press 'e' to output midi cc 11 to dac1");
            Serial.println("Press 'p' to toggle showing pwm values");
            Serial.println("Press '[' / ']' to raise / lower deadband min");
            Serial.println("Press '{' / '}' to rase / lower deadband max");

            break;

        case '[':
            // lower the bottom end of the deadband range for 4096 mapping
            if (deadbandMinMax4096.min > 0)
            {
                deadbandMinMax4096.min -= 1;
                markDeadbandMinMax4096Changed();
                Serial.println("Lowered deadbandMinMax4096.min to: " + String(deadbandMinMax4096.min));
            }
            break;
        case ']':
            // raise the bottom end of the deadband range for 4096 mapping
            deadbandMinMax4096.min += 1;
            markDeadbandMinMax4096Changed();
            Serial.println("Raised deadbandMinMax4096.min to: " + String(deadbandMinMax4096.min));
            break;
        case '{':
            // lower the bottom end of the deadband range for 4096 mapping
            deadbandMinMax4096.max -= 1;
            markDeadbandMinMax4096Changed();
            Serial.println("Lowered deadbandMinMax4096.max to: " + String(deadbandMinMax4096.max));
            break;
        case '}':
            // raise the bottom end of the deadband range for 4096 mapping
            if (deadbandMinMax4096.max < 4095)
            {
                deadbandMinMax4096.max += 1;
                markDeadbandMinMax4096Changed();
                Serial.println("Raised deadbandMinMax4096.max to: " + String(deadbandMinMax4096.max));
            }
            break;

        case '9':
            // send lower bound (value 0)
            Serial.println("sending lower bound value (0)");
            bitmashSendChannel = NOOD_CALIB_LOWER_BOUND_VALUE;
            break;
        case '0':
            // send upper bound (value 1023)
            Serial.println("upper lower bound value (1023)");
            bitmashSendChannel = NOOD_CALIB_UPPER_BOUND_VALUE;
            break;
        }
    }

    // don't know if this is necessary but I always flush the serial buffer
    while (Serial.available() > 0)
    {
        Serial.read();
    }
}

void updateSerialPrintValues()
{
    if (millis() - prevSerialPrintMills > serialPrintInterval)
    {
        serialPrintDebugValues();

        prevSerialPrintMills = millis();
    }
}
void serialPrintDebugValues()
{

    //*
    Serial.print("audioVal: " + String(audioVal));
    Serial.print(", aux1_val: " + String(aux1_val));
    Serial.print(", aux2_val: " + String(aux2_val));
    Serial.print(", aux3_val: " + String(aux3_val));
    Serial.println(", ledPWMVal: " + String(ledPWMVal));
    //*/

    //*
    Serial.print("noods: ");
    Serial.print(bitmash_nood1a + String(", "));
    Serial.print(bitmash_nood1b + String(", "));
    Serial.print(bitmash_nood2a + String(", "));
    Serial.println(bitmash_nood2b + String(", "));
    Serial.print("bitmashed_outs: nood1a: ");
    Serial.print(bitmashed_outs[0]);
    Serial.print(", nood1b: ");
    Serial.print(bitmashed_outs[1]);
    Serial.print(", nood2a: ");
    Serial.print(bitmashed_outs[2]);
    Serial.print(", nood2b: ");
    Serial.println(bitmashed_outs[3]);
    Serial.print(" -> bitmashed_outs (BIN): ");
    Serial.print(bitmashed_outs[0], BIN);
    Serial.print(", ");
    Serial.print(bitmashed_outs[1], BIN);
    Serial.print(", ");
    Serial.print(bitmashed_outs[2], BIN);
    Serial.print(", ");
    Serial.println(bitmashed_outs[3], BIN);
    //*/

    Serial.println();
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Serial.println("Receive Note On >>  channel: " + String(channel) + ", pitch: " + String(pitch) + ", velocity: " + String(velocity));

    if (pitch == NOOD_CALIB_TRIGGER_NOTE && velocity > 0)
    {
        Serial.println("Triggering calibration sequence via MIDI note-on");
        triggerNoodCalibSequence();
    }
}

void handleControlChange(byte channel, byte data1, byte data2)
{
    // Serial.println("Receive CC >>  channel: " + String(channel) + ", data1: " + String(data1) + ", data2: " + String(data2));

    // n00d individually addressing
    if (channel == 1)
    {
        switch (data1)
        {
        case 10:
            cc10_val = data2;
            break;
        case 11:
            cc11_val = map(data2, 0, 127, 0, 1023);
            break;
        case 30:
            bitmash_nood1a = data2;
            break;
        case 31:
            bitmash_nood1b = data2;
            break;
        case 32:
            bitmash_nood2a = data2;
            break;
        case 33:
            bitmash_nood2b = data2;
            break;
        }
    }

    if (channel == 1 && data1 == 0)
    {
        audioVal = map(data2, 0, 127, 0, 4095);
        audioVal = mapToActualMinMax_4096(audioVal, DEADBAND_INPUT_RANGE::_4096);

        // audioVal = mapToActualMinMax_256(data2, DEADBAND_INPUT_RANGE::_256) * 2;
        // if (audioVal > 128)
        // {
        //     audioVal++; // cheap way to make midi max 127 map to analog max 255
        // }
    }

    if (channel == 1 && data1 == 0)
    {
        // dac1_out = map(data2, 0, 127, 0, 4095);
        // if (audioVal > 128)
        // {
        //     audioVal++; // cheap way to make midi max 127 map to analog max 255
        // }
    }

    if (channel == 1 && data1 == 1)
    {
        // dac2_out = map(data2, 0, 127, 0, 4095);
        // bodyExpressionVal = data2 * 2;
        // if (bodyExpressionVal > 128)
        // {
        //     bodyExpressionVal++; // cheap way to make midi max 127 map to analog max 255
        //     }
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
}

uint8_t mapToActualMinMax_256(uint8_t val, uint8_t range)
{
    switch (range)
    {
    case DEADBAND_INPUT_RANGE::_256:
        return constrain(map(val, 0, 127, deadbandMinMax256.min, deadbandMinMax256.max), 0, 127);
        break;
    }
}

uint16_t mapToActualMinMax_1024(uint16_t val, uint16_t range)
{
    switch (range)
    {
    case DEADBAND_INPUT_RANGE::_1024:
        // return constrain(map(val, 0, 1023, 127, 936), 0, 1023);
        return constrain(map(val, 0, 1023, deadbandMinMax1024.min, deadbandMinMax1024.max), 0, 1023);
        break;
    }
}

uint16_t mapToActualMinMax_4096(uint16_t val, uint16_t range)
{
    switch (range)
    {
    case DEADBAND_INPUT_RANGE::_4096:
        // return constrain(map(val, 0, 4095, 127, 3968), 0, 4095);
        return constrain(map(val, 0, 4095, deadbandMinMax4096.min, deadbandMinMax4096.max), 0, 4095);
        break;
    }
}

void loadDeadbandMinMax4096FromFlash()
{
    DeadbandMinMax4096Flash stored;
    deadbandMinMax4096FlashStore.read(stored);

    if (stored.signature == DEADBAND_MIN_MAX_4096_FLASH_SIGNATURE)
    {
        deadbandMinMax4096.min = stored.min;
        deadbandMinMax4096.max = stored.max;
        Serial.println("Loaded deadbandMinMax4096 from flash -> min: " + String(deadbandMinMax4096.min) + ", max: " + String(deadbandMinMax4096.max));
    }
    else
    {
        Serial.println("No saved deadbandMinMax4096 found in flash, using defaults -> min: " + String(deadbandMinMax4096.min) + ", max: " + String(deadbandMinMax4096.max));
    }
}

void markDeadbandMinMax4096Changed()
{
    deadbandMinMax4096Dirty = true;
    deadbandMinMax4096LastChangeMillis = millis();
}

void updateDeadbandMinMax4096FlashSave()
{
    if (deadbandMinMax4096Dirty && (millis() - deadbandMinMax4096LastChangeMillis > deadbandMinMax4096SaveDelay))
    {
        DeadbandMinMax4096Flash toStore;
        toStore.min = deadbandMinMax4096.min;
        toStore.max = deadbandMinMax4096.max;
        toStore.signature = DEADBAND_MIN_MAX_4096_FLASH_SIGNATURE;
        deadbandMinMax4096FlashStore.write(toStore);

        deadbandMinMax4096Dirty = false;
        Serial.println("Saved deadbandMinMax4096 to flash -> min: " + String(deadbandMinMax4096.min) + ", max: " + String(deadbandMinMax4096.max));
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
