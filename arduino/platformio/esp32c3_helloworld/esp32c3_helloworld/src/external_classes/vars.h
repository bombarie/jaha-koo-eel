// setting PWM properties
const uint32_t freq = 2500;
const uint8_t resolution = 8;

enum HEAD_STATE
{
  STATE_1,
  STATE_2,
  STATE_3
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
  DISCONNECTED,
  CONNECTION_ESTABLISHED,
  CONNECTION_LOST,
  CONNECTED
};
CONNECTION_STATE connectionState = DISCONNECTED;
