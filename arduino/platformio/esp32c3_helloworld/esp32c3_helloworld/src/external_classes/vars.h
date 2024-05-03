// setting PWM properties
const uint32_t freq = 2500;
const uint8_t resolution = 8;

bool printSbusData = false;

enum HEAD_STATE
{
  STATE_1,
  STATE_2,
  STATE_3,
  STATE_4
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
