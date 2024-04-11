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

#include "player_input.hpp"
#include "connection_handler.hpp"

void readInput();
void handleConnectionRequest();

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
// Connection handler
std::shared_ptr<InputHandler> g_inputHandlerPtr = std::make_shared<InputHandler>(g_canBus, g_readTimer);
ConnectionHandler g_connectionHandler{g_canBus, g_readTimer, g_inputHandlerPtr};

void updateState()
{
  // update the leds based on the active player
  std::vector<int> connectedPlayers = g_inputHandlerPtr->getConnectedPlayers();

  for (int i = 0; i < g_numPlayers; i++)
  {
    if (std::find(connectedPlayers.begin(), connectedPlayers.end(), i) != connectedPlayers.end())
    {
      digitalWrite(g_playerPins[i], HIGH);
    }
    else
    {
      digitalWrite(g_playerPins[i], LOW);
    }
  }

  g_inputHandlerPtr->tick();
}

bool high = false;
void testTask()
{
  // turn player 1's led on and off
  if (high)
  {
    digitalWrite(PLAYER_1_STATUS_PIN, HIGH);
  }
  else
  {
    digitalWrite(PLAYER_1_STATUS_PIN, LOW);
  }

  high = !high;
}

void canBusTick()
{
  g_canBus.Tick();
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

  // initialize the connection and input handlers
  g_connectionHandler.initialize();
  g_inputHandlerPtr->initialize();

  // initialize the CAN bus
  g_canBus.Initialize(ICAN::BaudRate::kBaud1M);
  g_readTimer.AddTimer(100, updateState);
  g_readTimer.AddTimer(100, testTask);
  g_readTimer.AddTimer(100, canBusTick);
}

void loop()
{
  g_readTimer.Tick(millis());
}