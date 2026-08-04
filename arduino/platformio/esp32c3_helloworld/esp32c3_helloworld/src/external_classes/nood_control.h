#include <Preferences.h>

int16_t noodVals[] = {0, 0, 0, 0};
// kept as float (not int16_t): truncating this on every EMA step causes the average to
// permanently stall short of its target once the per-step increment drops below 1 unit
// (e.g. with noodsAvgSmoothFactor=0.95 and a target of 255, it locks up at exactly 236)
float noodAvgVals[] = {0, 0, 0, 0};

// this is the value amount that we subtract from 127, to allow some deadband between the nood values.
// Effectively, determines the resolution of the noods. 0 is no deadband, 127 is max deadband.
#define NOOD_VALUES_TRANSMISSION_BANDWIDTH 24
#define NOOD_VALUES_BANDWIDTH (127 - NOOD_VALUES_TRANSMISSION_BANDWIDTH)

#define n00d_1a_Pin D8
#define n00d_1b_Pin D1
#define n00d_2a_Pin D10
#define n00d_2b_Pin D0

static const uint8_t nood1a_chan = 0;
static const uint8_t nood1b_chan = 1;
static const uint8_t nood2a_chan = 2;
static const uint8_t nood2b_chan = 3;

// state machine driving the calibration sequence in doThresholdChangeCalibrated()
enum CalibState : int8_t
{
  CALIB_IDLE = 0,    // regular parsing using the last calibrated bounds
  CALIB_LOW,         // sampling+adjusting calibLowerBound
  CALIB_HIGH,        // sampling+adjusting calibUpperBound
  CALIB_AWAIT_RESUME // waiting for throttle to rise back above threshold before resuming regular parsing
};
CalibState calibState = CALIB_IDLE;
CalibState prevCalibState;

void initNoods();
void updateBodyLightValues();
void setBodyLights();
void setn00d(uint8_t chan, uint8_t val);

// calibLowerBound/calibUpperBound are defined further down, persistence helpers need them earlier
extern int16_t calibLowerBound;
extern int16_t calibUpperBound;
void loadCalibBounds();
void checkAndPersistCalibBounds();

void initNoods()
{
  pinMode(n00d_1a_Pin, OUTPUT);
  pinMode(n00d_1b_Pin, OUTPUT);
  pinMode(n00d_2a_Pin, OUTPUT);
  pinMode(n00d_2b_Pin, OUTPUT);

  // nood 1a
  ledcSetup(nood1a_chan, freq, resolution);
  ledcAttachPin(n00d_1a_Pin, nood1a_chan);

  // nood 1b
  ledcSetup(nood1b_chan, freq, resolution);
  ledcAttachPin(n00d_1b_Pin, nood1b_chan);

  // nood 2a
  ledcSetup(nood2a_chan, freq, resolution);
  ledcAttachPin(n00d_2a_Pin, nood2a_chan);

  // nood 2b
  ledcSetup(nood2b_chan, freq, resolution);
  ledcAttachPin(n00d_2b_Pin, nood2b_chan);

  setn00d(nood1a_chan, 0);
  setn00d(nood1b_chan, 0);
  setn00d(nood2a_chan, 0);
  setn00d(nood2b_chan, 0);

  prevCalibState = calibState;

  loadCalibBounds();
}

// static const uint8_t noodIncomingValueOffsets[] = {9, 10, 9, 9}; // original values
static const uint8_t noodIncomingValueOffsets[] = {0, 0, 0, 0}; // original values
// static const uint8_t noodIncomingValueOffsets[] = {11, 10, 10, 10}; // 2026-06 adjusted values
// static const uint8_t noodIncomingValueOffsets[] = {16, 12, 13, 0}; // 2026-06 adjusted values

static constexpr int8_t noodSegmentIdentifiers[] = {0b100, 0b101, 0b110, 0b111}; // upper bits of the nood values
int8_t currSelectedChannel = 0;                                                  // 0-3 for nood1a, nood1b, nood2a, nood2b, -1 for None
int8_t currDetectedChannelSmooth = 0;                                            // 0-3 for nood1a, nood1b, nood2a, nood2b, -1 for None
int8_t currDetectedChannel = 0;
int8_t prevDetectedChannel = 0; // 0-3 for nood1a, nood1b, nood2a, nood2b, -1 for None
int8_t nextChannelIndex = 1;
uint8_t channelChangeCounter = 0;

// crappy way to do this -> refactor using Enum
byte ELRSParseMethod = 3;

void doDirectChange();
void doThresholdChange();
void doThresholdChangeCalibrated();
void doNextItemChange();

float throttleSmoothFactor = 0.12;
int8_t channelChangeThreshold = 1; // number of consecutive readings of a new channel

// calibration state for doThresholdChangeCalibrated() - the sender intermittently transmits
// a lower-bound marker (top 3 bits == 0) followed by an upper-bound value (> 512), letting us
// track the actual analog range instead of trusting SBUS_VAL_MIN/SBUS_VAL_MAX to be exact.
int16_t calibLowerBound = 0;
int16_t calibUpperBound = 1023;
float calibSmoothFactor = 0.9; // smooths new marker readings into the stored bound

// persistence for calibLowerBound/calibUpperBound (NVS via Preferences, the modern
// replacement for the old EEPROM emulation lib): debounced so we don't wear out
// flash while calibration is actively converging every frame.
Preferences noodCalibPrefs;
static const char *NOOD_CALIB_PREFS_NAMESPACE = "noodcal";
static const unsigned long CALIB_BOUNDS_SAVE_DEBOUNCE_MS = 5000;

int16_t calibLowerBoundPrev = calibLowerBound;
int16_t calibUpperBoundPrev = calibUpperBound;
unsigned long calibBoundsChangedAt = 0;
bool calibBoundsPendingSave = false;

void loadCalibBounds()
{
  Serial.println("Loading radio-link calibration bounds from NVS...");

  noodCalibPrefs.begin(NOOD_CALIB_PREFS_NAMESPACE, true); // read-only
  calibLowerBound = noodCalibPrefs.getShort("calibLow", calibLowerBound);
  calibUpperBound = noodCalibPrefs.getShort("calibHigh", calibUpperBound);
  noodCalibPrefs.end();

  calibLowerBoundPrev = calibLowerBound;
  calibUpperBoundPrev = calibUpperBound;
}

void saveCalibBounds()
{
  Serial.println("Saving radio-link calibration bounds to NVS...");

  noodCalibPrefs.begin(NOOD_CALIB_PREFS_NAMESPACE, false); // read-write
  noodCalibPrefs.putShort("calibLow", calibLowerBound);
  noodCalibPrefs.putShort("calibHigh", calibUpperBound);
  noodCalibPrefs.end();
}

// call every frame; debounces writes so calibLowerBound/calibUpperBound only get
// persisted once they've held still for CALIB_BOUNDS_SAVE_DEBOUNCE_MS
void checkAndPersistCalibBounds()
{
  if (calibLowerBound != calibLowerBoundPrev || calibUpperBound != calibUpperBoundPrev)
  {
    calibLowerBoundPrev = calibLowerBound;
    calibUpperBoundPrev = calibUpperBound;
    calibBoundsChangedAt = millis();
    calibBoundsPendingSave = true;
  }
  else if (calibBoundsPendingSave && (millis() - calibBoundsChangedAt >= CALIB_BOUNDS_SAVE_DEBOUNCE_MS))
  {
    saveCalibBounds();
    calibBoundsPendingSave = false;
  }
}

int16_t calibratedThrottle = throttle;
int16_t calibratedThrottleSmoothed = throttleSmoothed;
int16_t calibThreshold = 0.25 * (calibUpperBound - calibLowerBound);

uint16_t calibThrottleSmoothed = 0;
int8_t calibDetectedChannel = 0;
int8_t calibDetectedChannelSmooth = 0;

// DEBUG
bool bListenForIncomingCalibration = true;

float noodsAvgSmoothFactor = 0.95;

void updateBodyLightValues()
{
  // map throttle range to 0-1023
  throttle = map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1023);
  throttleSmoothed = throttleSmoothFactor * throttleSmoothed + (1.0 - throttleSmoothFactor) * throttle;

  currDetectedChannelSmooth = int8_t(throttleSmoothed >> 7) - 4; // upper 3 bits of the throttle value determine the nood channel
  currDetectedChannel = int8_t(throttle >> 7) - 4;

  // These are different approaches to parsing the raw ELRS input data
  switch (ELRSParseMethod)
  {
  case 0:
    // no clever waiting for the next logical channel, just direct parsing of the received input
    doDirectChange();
    break;
  case 1:
    // only change channel if multiple frames point to that channel consistently
    doThresholdChange();
    break;
  case 2:
    // Like doThresholdChange(), but this algorithm only accepts the logical 'next channel', and discards anything different.
    doNextItemChange();
    break;
  case 3:
    // Like doThresholdChange(), but continuously recalibrates against observed lower/upper bound markers sent by the transmitter.
    doThresholdChangeCalibrated();
    break;
  }

  checkAndPersistCalibBounds();

  // smoothed values
  noodAvgVals[0] = noodsAvgSmoothFactor * noodAvgVals[0] + (1.0 - noodsAvgSmoothFactor) * noodVals[0];
  noodAvgVals[1] = noodsAvgSmoothFactor * noodAvgVals[1] + (1.0 - noodsAvgSmoothFactor) * noodVals[1];
  noodAvgVals[2] = noodsAvgSmoothFactor * noodAvgVals[2] + (1.0 - noodsAvgSmoothFactor) * noodVals[2];
  noodAvgVals[3] = noodsAvgSmoothFactor * noodAvgVals[3] + (1.0 - noodsAvgSmoothFactor) * noodVals[3];
}

void doDirectChange()
{
  int16_t newVal = (throttle & 0x7F) - noodIncomingValueOffsets[currDetectedChannel];
  newVal = map(constrain(newVal, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

  noodVals[currDetectedChannel] = newVal;
}

void doThresholdChange()
{
  if (currDetectedChannelSmooth != currSelectedChannel)
  {
    if (channelChangeCounter < channelChangeThreshold)
    {
      channelChangeCounter++;
    }
    else
    {
      currSelectedChannel = currDetectedChannelSmooth;

      // int16_t newVal = (thr & 0x7F) - noodIncomingValueOffsets[currSelectedChannel];
      // newVal = map(constrain(newVal, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

      // noodVals[currSelectedChannel] = newVal;
    }
  }
  else
  {
    channelChangeCounter = 0;

    if (currDetectedChannel == currSelectedChannel)
    {
      int16_t newVal = (throttle & 0x7F) - noodIncomingValueOffsets[currSelectedChannel];
      newVal = map(constrain(newVal, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

      noodVals[currSelectedChannel] = newVal;
    }
  }

  // prevDetectedChannel = currDetectedChannel;
}

void doThresholdChangeCalibrated()
{
  /*
  first: when LOW condition was detected -> as long as it's LOW and value not >512 -> keep sampling and adjusting calibLowerBound
  then, as long as HIGH condition was detected (throttle > 512) and the current throttle is not lower than (0.25 * (calibUpperBound - calibLowerBound)) -> keep sampling and adjusting calibUpperBound
  then, after throttle value is again higher than (0.25 * (calibUpperBound - calibLowerBound)) -> we assume calibLowerBound and calibUpperBound are set, and the regular parsing continues.
   */

  if (bListenForIncomingCalibration)
  {

    bool isLowCondition = (throttle >> 7 == 0); // top 3 bits are 0

    if (calibState == CALIB_IDLE && isLowCondition)
    {
      calibState = CALIB_LOW;
    }

    switch (calibState)
    {
    case CALIB_LOW:
      if (isLowCondition && throttleSmoothed <= 512)
      {
        calibLowerBound = calibSmoothFactor * calibLowerBound + (1.0 - calibSmoothFactor) * throttleSmoothed;
      }
      else if (throttleSmoothed > 512)
      {
        calibState = CALIB_HIGH;
        calibUpperBound = calibSmoothFactor * calibUpperBound + (1.0 - calibSmoothFactor) * throttleSmoothed;
      }

      // update calibThreshold for the next frame, in case calibLowerBound or calibUpperBound changed
      calibThreshold = 0.25 * (calibUpperBound - calibLowerBound);
      return;

    case CALIB_HIGH:
      if (throttleSmoothed > 512 && throttleSmoothed >= calibThreshold)
      {
        calibUpperBound = calibSmoothFactor * calibUpperBound + (1.0 - calibSmoothFactor) * throttleSmoothed;
      }
      else
      {
        calibState = CALIB_AWAIT_RESUME;
      }

      // update calibThreshold for the next frame, in case calibLowerBound or calibUpperBound changed
      calibThreshold = 0.25 * (calibUpperBound - calibLowerBound);
      return;

    case CALIB_AWAIT_RESUME:
      if (throttleSmoothed > calibThreshold)
      {
        calibState = CALIB_IDLE; // bounds are assumed set -> fall through and resume regular parsing this frame
      }
      else
      {
        return;
      }
      break;

    case CALIB_IDLE:
    default:
      break;
    }
  }

  // so.... now that we have calibrated the bounds, we can use them to remap the raw throttle value to a 0-1023 range, and then decode the channel and value from that.

  // remap the raw throttle through the observed bounds before decoding channel + value,
  // instead of trusting SBUS_VAL_MIN/SBUS_VAL_MAX (and the resulting 0-1023 range) to be exact
  if (calibUpperBound > calibLowerBound)
  {
    calibratedThrottle = map(throttle, calibLowerBound, calibUpperBound, 0, 1023);
    calibratedThrottle = constrain(calibratedThrottle, 0, 1023);

    calibratedThrottleSmoothed = map(throttleSmoothed, calibLowerBound, calibUpperBound, 0, 1023);
    calibratedThrottleSmoothed = constrain(calibratedThrottleSmoothed, 0, 1023);

    // after remapping, we can decode the channel and value from the calibrated throttle
    currDetectedChannelSmooth = int8_t(calibratedThrottleSmoothed >> 7) - 4; // upper 3 bits of the throttle value determine the nood channel
    currDetectedChannel = int8_t(calibratedThrottle >> 7) - 4;
  }

  calibThrottleSmoothed = throttleSmoothFactor * calibratedThrottleSmoothed + (1.0 - throttleSmoothFactor) * calibratedThrottle;

  calibDetectedChannelSmooth = int8_t(calibThrottleSmoothed >> 7) - 4;
  calibDetectedChannel = int8_t(calibratedThrottle >> 7) - 4;

  if (calibDetectedChannelSmooth != currSelectedChannel)
  {
    if (channelChangeCounter < channelChangeThreshold)
    {
      channelChangeCounter++;
    }
    else
    {
      currSelectedChannel = calibDetectedChannelSmooth;
    }
  }
  else
  {
    channelChangeCounter = 0;

    if (calibDetectedChannel == currSelectedChannel)
    {
      int16_t newVal = (calibratedThrottle & 0x7F) - noodIncomingValueOffsets[currSelectedChannel];
      newVal = map(constrain(newVal, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

      noodVals[currSelectedChannel] = newVal;
    }
  }
}

void doNextItemChange()
{
  if (currDetectedChannelSmooth == nextChannelIndex)
  {
    if (channelChangeCounter < channelChangeThreshold)
    {
      channelChangeCounter++;
    }
    else
    {
      currSelectedChannel = currDetectedChannelSmooth;

      nextChannelIndex++;
      nextChannelIndex = nextChannelIndex % 4;
    }
  }
  else
  {
    channelChangeCounter = 0;
  }

  if (currDetectedChannel == currSelectedChannel)
  {
    int16_t newVal = (throttle & 0x7F) - noodIncomingValueOffsets[currSelectedChannel];
    newVal = map(constrain(newVal, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);

    // Let's see if this filtering approach works....
    // If there's a small diff, take new val right away.
    // If there's a big jump, lerp towards it.
    // The idea is that this should help prevent sudden glitches, resulting from one channel's value being wrongfully overwritten by another channel's value
    if (
        (newVal > noodVals[currSelectedChannel]) &&
        abs(noodVals[currSelectedChannel] - newVal) > 100)
    {
      int16_t _newVal = 0.9 * noodVals[currSelectedChannel] + 0.1 * newVal;
      Serial.println("ch " + String(currSelectedChannel) + " made big jump >> lerping. Raw newVal: " + String(newVal) + " >> now going " + String(noodVals[currSelectedChannel]) + " -> " + String(_newVal));
      noodVals[currSelectedChannel] = _newVal;
    }
    else
    {
      noodVals[currSelectedChannel] = newVal;
    }
  }
}

void setBodyLights()
{
  setn00d(nood1a_chan, uint8_t(noodAvgVals[0]));
  setn00d(nood1b_chan, uint8_t(noodAvgVals[1]));
  setn00d(nood2a_chan, uint8_t(noodAvgVals[2]));
  setn00d(nood2b_chan, uint8_t(noodAvgVals[3]));
}

void setn00d(uint8_t chan, uint8_t val)
{
  ledcWrite(chan, (255 - val)); // original
}
