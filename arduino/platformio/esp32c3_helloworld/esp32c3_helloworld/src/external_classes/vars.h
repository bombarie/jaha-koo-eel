// setting PWM properties
const uint32_t freq = 2500;
const uint8_t resolution = 8;

bool printSbusData = false;

int16_t nood1a, nood1b, nood2a, nood2b;
uint16_t throttle, throttleAdjusted, throttleSmoothed;

enum HEAD_STATE
{
  STATE_1,  // all off
  STATE_2,  // all green
  STATE_3,  // all white
  STATE_4   // all green eyes, white mouth
};
HEAD_STATE headState = STATE_1;

enum BODY_STATE
{
  NOOD1,
  NOOD2,
  BOTH_NOODS
};
BODY_STATE bodyState = BOTH_NOODS;

// The CONNECTION_ESTABLISHED and CONNECTION_LOST states are kind of vanity states, in case I want to
// implement some kind of unique visual feedback for when the connection is lost or established.
enum CONNECTION_STATE
{
  DISCONNECTED = 0,
  CONNECTION_ESTABLISHED = 1,
  CONNECTION_LOST = 2,
  CONNECTED = 3
};
CONNECTION_STATE connectionState = DISCONNECTED;
