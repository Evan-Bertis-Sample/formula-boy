//
// formula-boy
// controller bus for interacting with controllers
// created by Evan Bertis-Sample
//

// Expected CAN message format
// message_type 0 : controller input (from controller to game)
//   signal 0 : player_id (int8_t)
//   signal 1 : vertical axis (float -1.0 to 1.0)
//   signal 2 : horizontal axis (float -1.0 to 1.0)
//   signal 3 : rotation axis (float -1.0 to 1.0)
//   signal 4 : button bitmask (uint8_t)
//     (in order from least significant bit to most significant bit)
//     bit 0 : shoot
//     bit 1 : mine
//     bit 2 : select
//     bit 3 : back

// message_type 1 : connection request/acknowledgement (from controller to game)
//   signal 0 : player_id (int)
//     (if signal 0 is -1, the message is a request, otherwise it is an acknowledgement with the player_id)

// message_type 2 : connection response (from game to controller)
//   signal 0 : player_id (int)

// Important addresses
// 0x000: Connection Message/Acknowledgement (controller to game)
// 0x100: Connection Response (game to controller)
// 0x200: Controller Input (controller to game)

#include <Arduino.h>
#include <CAN.h>

void readInput();

// Pin Definitions
#define PLAYER_1_STATUS_PIN GPIO_NUM_32
#define PLAYER_2_STATUS_PIN GPIO_NUM_33
#define PLAYER_3_STATUS_PIN GPIO_NUM_25

int g_playerPins[] = {PLAYER_1_STATUS_PIN, PLAYER_2_STATUS_PIN, PLAYER_3_STATUS_PIN};
int g_numPlayers = 3;
int g_activePlayer = 0;

// CAN setup
// TX pin and RX pin as input.
CAN g_canBus{};

// Structure for handling timers
VirtualTimerGroup g_readTimer;

// CAN addresses
const uint32_t g_controllerConnectionAddress = 0x000;
const uint32_t g_controllerConnectionResponseAddress = 0x100;
const uint32_t g_controllerInputAddress = 0x200;

// CAN signals
MakeSignedCANSignal(int8_t, 0, 8, 1, 0) g_playerIdSignal{};           // one byte
MakeSignedCANSignal(float, 8, 16, 0.01, 0) g_verticalAxisSignal{};    // two bytes
MakeSignedCANSignal(float, 24, 16, 0.01, 0) g_horizontalAxisSignal{}; // two bytes
MakeSignedCANSignal(float, 40, 16, 0.01, 0) g_rotationAxisSignal{};   // two bytes
MakeUnsignedCANSignal(uint8_t, 56, 8, 1, 0) g_buttonBitmaskSignal{}; // one byte

// CAN message
CANRXMessage<5> g_controllerInputMessage{g_canBus,
                                         g_controllerInputAddress,
                                         readInput,
                                         g_playerIdSignal,
                                         g_verticalAxisSignal,
                                         g_horizontalAxisSignal,
                                         g_rotationAxisSignal,
                                         g_buttonBitmaskSignal};


void updateState()
{
  // write to the active player's pin
  digitalWrite(g_playerPins[g_activePlayer], LOW);
  g_activePlayer = (g_activePlayer + 1) % g_numPlayers;
  digitalWrite(g_playerPins[g_activePlayer], HIGH);
  Serial.println("Player " + String(g_activePlayer + 1) + " is active");
  // move to the next player
}


void setup()
{
  // initialize the pins
  for (int i = 0; i < g_numPlayers; i++)
  {
    pinMode(g_playerPins[i], OUTPUT);
    digitalWrite(g_playerPins[i], LOW);
  }

  Serial.begin(9600);
  Serial.println("Starting game");

  // initialize the CAN bus
  g_canBus.Initialize(ICAN::BaudRate::kBaud1M);

  g_readTimer.AddTimer(1000, updateState);
}

void loop()
{
  g_readTimer.Tick(millis());
}

void readInput()
{
  Serial.println("Reading input");
  // get the player id
  int8_t playerId = g_playerIdSignal;
  Serial.println("Player id: " + String(playerId));
}