int16_t noodVals[] = {0, 0, 0, 0};
int16_t noodAvgVals[] = {0, 0, 0, 0};

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

void initNoods();
void updateBodyLightValues();
void setBodyLights();
void setn00d(uint8_t chan, uint8_t val);

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
}

// static const uint8_t noodIncomingValueOffsets[] = {9, 10, 9, 9}; // original values
static const uint8_t noodIncomingValueOffsets[] = {0, 0, 0, 0}; // original values
// static const uint8_t noodIncomingValueOffsets[] = {11, 10, 10, 10}; // 2026-06 adjusted values
// static const uint8_t noodIncomingValueOffsets[] = {16, 12, 13, 0}; // 2026-06 adjusted values

static constexpr int8_t noodSegmentIdentifiers[] = {0b100, 0b101, 0b110, 0b111}; // upper bits of the nood values
int8_t currSelectedChannel = 0;                                                  // 0-3 for nood1a, nood1b, nood2a, nood2b, -1 for None
int8_t currDetectedChannelSmooth = 0;                                                  // 0-3 for nood1a, nood1b, nood2a, nood2b, -1 for None
int8_t currDetectedChannel = 0;
int8_t prevDetectedChannel = 0; // 0-3 for nood1a, nood1b, nood2a, nood2b, -1 for None
int8_t nextChannelIndex = 1;
uint8_t channelChangeCounter = 0;

// crappy way to do this -> refactor using Enum
byte ELRSParseMethod = 2;

void doDirectChange();
void doThresholdChange();
void doNextItemChange();

float throttleSmoothFactor = 0.12;
int8_t channelChangeThreshold = 1; // number of consecutive readings of a new channel

float noodsAvgSmoothFactor = 0.95;

uint16_t throttleSmoothed = 0;
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
    doDirectChange();
    break;
  case 1:
    doThresholdChange();
    break;
  case 2:
    doNextItemChange();
    break;
  }

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
  setn00d(nood1a_chan, noodAvgVals[0]);
  setn00d(nood1b_chan, noodAvgVals[1]);
  setn00d(nood2a_chan, noodAvgVals[2]);
  setn00d(nood2b_chan, noodAvgVals[3]);
}

void setn00d(uint8_t chan, uint8_t val)
{
  ledcWrite(chan, (255 - val)); // original
}
