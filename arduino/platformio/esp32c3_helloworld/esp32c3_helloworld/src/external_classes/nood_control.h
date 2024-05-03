int16_t noodVals[] = {0, 0, 0, 0};
int16_t noodAvgVals[] = {0, 0, 0, 0};
uint16_t n00dSegmentIdentifiers[] = {512, 640, 768, 896};

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

void updateBodyLightValues()
{
  // map throttle range to 0-1023
  throttle = map(data.ch[TX_THROTTLE], SBUS_VAL_MIN, SBUS_VAL_MAX, 0, 1023);

  throttleAdjusted = 0; // used to store the adjusted throttle value

  // if (throttle & (0x1 << 9)) // 512
  // if (throttle & n00dSegmentIdentifiers[0] == n00dSegmentIdentifiers[0]) // 512
  if (throttle >> 7 == 0x4) // 0b100
  {
    // throttleAdjusted = throttle - (throttle >> 5);
    throttleAdjusted = throttle - 9;
    n00d1a = (throttle & 0x7F) - 9;
    n00d1a = map(constrain(n00d1a, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);
  }
  // if (throttle & (0x1 << 8)) // 256
  // if (throttle & n00dSegmentIdentifiers[1] == n00dSegmentIdentifiers[1]) // 640
  if (throttle >> 7 == 0x5) // 0b101
  {
    // throttle -= throttle - (throttle >> 5);
    throttleAdjusted = throttle - 10;
    n00d1b = (throttle & 0x7F) - 10;
    n00d1b = map(constrain(n00d1b, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);
  }
  // if (throttle & (0x1 << 7)) // 128
  // if (throttle & n00dSegmentIdentifiers[2] == n00dSegmentIdentifiers[2]) // 768
  if (throttle >> 7 == 0x6) // 0b110
  {
    throttleAdjusted = throttle - 9;
    n00d2a = (throttle & 0x7F) - 9;
    n00d2a = map(constrain(n00d2a, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);
  }
  // if (throttle & (0x1 << 6)) // 64
  // if (throttle & n00dSegmentIdentifiers[3] == n00dSegmentIdentifiers[3]) // 896
  if (throttle >> 7 == 0x7) // 0b111
  {
    throttleAdjusted = throttle - 9;
    n00d2b = (throttle & 0x7F) - 9;
    n00d2b = map(constrain(n00d2b, 0, NOOD_VALUES_BANDWIDTH), 0, NOOD_VALUES_BANDWIDTH, 0, 255);
  }

  noodVals[0] = n00d1a;
  noodVals[1] = n00d1b;
  noodVals[2] = n00d2a;
  noodVals[3] = n00d2b;

  noodAvgVals[0] = 0.85 * noodAvgVals[0] + 0.15 * noodVals[0];
  noodAvgVals[1] = 0.85 * noodAvgVals[1] + 0.15 * noodVals[1];
  noodAvgVals[2] = 0.85 * noodAvgVals[2] + 0.15 * noodVals[2];
  noodAvgVals[3] = 0.85 * noodAvgVals[3] + 0.15 * noodVals[3];
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
  ledcWrite(chan, (255 - val));
}

