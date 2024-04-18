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
MakeUnsignedCANSignal(uint64_t, 0, 64, 1, 0) g_connectionResponseSignal{}; // 8 bytes
CANRXMessage<1> g_connectionResponseMessage{
    g_canBus,
    CONTROLLER_CONNECTION_RESPONSE_ADDRESS,
    []()
    {
      Serial.println("Connection response received");
      handleConnectionResponse();
    },
    g_connectionResponseSignal,
};

// Player Input Message
MakeSignedCANSignal(int8_t, 0, 8, 1, 0) g_playerIdSignal{};           // one byte
MakeSignedCANSignal(float, 8, 16, 10, 0) g_verticalAxisSignal{};    // two bytes
MakeSignedCANSignal(float, 24, 16, 10, 0) g_horizontalAxisSignal{}; // two bytes
MakeSignedCANSignal(float, 40, 16, 10, 0) g_rotationAxisSignal{};   // two bytes
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
int8_t g_deviceId = 0xFF;

int8_t generateDeviceID()
{
  return (int8_t)random(0, 255);
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

  Serial.printf("Sending player inputs as player %d\n", g_playerId);
  Serial.printf("Vertical: %f\n", (float)g_verticalAxisSignal);
  Serial.printf("Horizontal: %f\n", (float)g_horizontalAxisSignal);
  Serial.printf("Rotation: %f\n", (float)g_rotationAxisSignal);
  Serial.printf("Button bitmask: %d\n", (int)g_buttonBitmaskSignal);
}

void handleConnectionResponse()
{
  Serial.println("Connection response received");
  // parse out the player id from the message
  // it is sent as a 64 bit unsigned integer, where the first byte is the device id
  // and the next byte is the player id
  // we need to keep in mind that the byte order is reversed in the message
  uint64_t data = g_connectionResponseSignal;
  // Serial.printf("Data: %llx\n", data);
  std::array<int8_t, 8> bytes;
  for (int i = 0; i < 8; i++)
  {
    bytes[i] = (int8_t)(data >> (i * 8));
    // Serial.printf("Byte %d: %x\n", i, bytes[i]);
  }
  // grab the msb of the data
  int8_t deviceId = bytes[0];
  int8_t playerId = bytes[1];
  Serial.printf("Player id: %d\n", playerId);
  Serial.printf("Device id: %d\n", deviceId);

  // if the device id matches the one we sent, we are connected
  if (deviceId == g_deviceId)
  {
    g_playerId = playerId;
    g_controllerState = ControllerState::CONNECTED;
    Serial.println("Connected");
    // g_connectionRequestMessage.Disable();
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
    Serial.printf("Sending connection request with device id: %d\n", g_deviceId);
    g_controllerState = ControllerState::AWAITING_CONNECTION_RESPONSE;
    break;
  case ControllerState::AWAITING_CONNECTION_RESPONSE:
    Serial.println("Awaiting connection response");
    break;
  case ControllerState::CONNECTED:
    // g_playerInputMessage.Enable();
    getPlayerInputs();
    break;
  }
  g_canBus.Tick();
}

#pragma endregion

void setup()
{
  g_canBus.Initialize(ICAN::BaudRate::kBaud1M);
  g_timerGroup.AddTimer(100, updateState);

  // initialize the player id signal to be -1
  g_playerIdSignal = -1;
  // generate a random device id
  g_deviceId = 10;
  g_connectionRequestDeviceIdSignal = g_deviceId;

  Serial.begin(9600);
  Serial.println("Started");
  Serial.println("Setup complete");
  // g_playerInputMessage.Disable();
}

void loop() { g_timerGroup.Tick(millis()); }