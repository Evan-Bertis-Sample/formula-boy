#include <Arduino.h>
#include <CAN.h>

VirtualTimerGroup g_timerGroup;

#pragma region CAN_Setup
CAN g_canBus{};
// addresses
const uint32_t CONTROLLER_CONNECTION_ADDRESS = 0x000;
const uint32_t CONTROLLER_CONNECTION_RESPONSE_ADDRESS = 0x100;
const uint32_t CONTROLLER_INPUT_ADDRESS = 0x200;

// Player Connection Request Message
MakeSignedCANSignal(int8_t, 0, 8, 1, 0) g_connectionRequestDeviceIdSignal{}; // one byte
CANTXMessage<1> g_connectionRequestMessage{
    g_canBus,
    CONTROLLER_CONNECTION_ADDRESS,
    1, 100, g_timerGroup,
    g_connectionRequestDeviceIdSignal};

// Player Connection Response Message
void handleConnectionResponse(); // forward declaration
MakeUnsignedCANSignal(uint64_t, 0, 64, 1, 0) g_connectionResponsePlayerIdSignal{}; // 8 bytes
CANRXMessage<1> g_connectionResponseMessage{
    g_canBus,
    CONTROLLER_CONNECTION_RESPONSE_ADDRESS,
    handleConnectionResponse,
    g_connectionRequestDeviceIdSignal,
};

// Player Input Message
MakeSignedCANSignal(int8_t, 0, 8, 1, 0) g_playerIdSignal{};           // one byte
MakeSignedCANSignal(float, 8, 16, 0.01, 0) g_verticalAxisSignal{};    // two bytes
MakeSignedCANSignal(float, 24, 16, 0.01, 0) g_horizontalAxisSignal{}; // two bytes
MakeSignedCANSignal(float, 40, 16, 0.01, 0) g_rotationAxisSignal{};   // two bytes
MakeUnsignedCANSignal(uint8_t, 56, 8, 1, 0) g_buttonBitmaskSignal{};  // one byte
CANTXMessage<5> g_playerInputMessage{
    g_canBus,
    CONTROLLER_INPUT_ADDRESS,
    1, 100, g_timerGroup,
    g_playerIdSignal,
    g_verticalAxisSignal,
    g_horizontalAxisSignal,
    g_rotationAxisSignal,
    g_buttonBitmaskSignal};

#pragma endregion

#pragma StateMachine
enum class ControllerState
{
  DISCONNECTED,
  AWAITING_CONNECTION_RESPONSE,
  CONNECTED
};

ControllerState g_controllerState = ControllerState::DISCONNECTED;
int8_t g_playerId = -1;
int8_t g_deviceId = -1;

int8_t generateDeviceID()
{
  return (int8_t)random(0, 127);
}

void getPlayerInputs()
{
  g_playerIdSignal = g_playerId;

  // imagine this is where we would get the player inputs in the hardware
  // for now we will just send some random inputs
  g_verticalAxisSignal = random(-100, 100) / 100.0f;
  g_horizontalAxisSignal = random(-100, 100) / 100.0f;
  g_rotationAxisSignal = random(-100, 100) / 100.0f;
  g_buttonBitmaskSignal = random(0, 255);
}

void handleConnectionResponse()
{
  Serial.println("Connection response received");
  // parse out the player id from the message
  // it is sent as a 64 bit unsigned integer, where the first byte is the device id
  // and the remaining 7 bytes are the player id
  uint64_t data = g_connectionResponsePlayerIdSignal;
  int8_t playerId = (int8_t)(data & 0xFF);
  int8_t deviceId = (int8_t)(data >> 8);
  Serial.printf("Player id: %d\n", playerId);
  Serial.printf("Device id: %d\n", deviceId);

  // if the device id matches the one we sent, we are connected
  if (deviceId == g_deviceId)
  {
    g_playerId = playerId;
    g_controllerState = ControllerState::CONNECTED;
    Serial.println("Connected");
  }
  else
  {
    Serial.println("Heard Response, but not connected");
  }
}

void updateState()
{
  switch (g_controllerState)
  {
  case ControllerState::DISCONNECTED:
    g_deviceId = generateDeviceID();
    g_connectionRequestDeviceIdSignal = g_deviceId;
    Serial.printf("Sending connection request with device id: %d\n", g_deviceId);
    g_controllerState = ControllerState::AWAITING_CONNECTION_RESPONSE;
    break;
  case ControllerState::AWAITING_CONNECTION_RESPONSE:
    Serial.println("Awaiting connection response");
    break;
  case ControllerState::CONNECTED:
    getPlayerInputs();
    break;
  }
}

#pragma endregion

void setup()
{
  g_canBus.Initialize(ICAN::BaudRate::kBaud1M);
  g_timerGroup.AddTimer(100, updateState);

  // initialize the player id signal to be -1
  g_playerIdSignal = -1;

  Serial.begin(9600);
  Serial.println("Started");
}

void loop() { g_timerGroup.Tick(millis()); }