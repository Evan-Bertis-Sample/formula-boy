#include <Arduino.h>
#include <CAN.h>

// CAN setup
// TX pin and RX pin as input.
CAN g_canBus{};
// Structure for handling timers
VirtualTimerGroup g_timerGroup;

// addresses
const uint32_t CONTROLLER_CONNECTION_ADDRESS = 0x000;

MakeSignedCANSignal(int8_t, 0, 8, 1, 0) g_connectionRequestPlayerIdSignal{}; // one byte

CANTXMessage<1> g_connectionRequestMessage{
    g_canBus,
    CONTROLLER_CONNECTION_ADDRESS,
    1, 100, g_timerGroup,
    g_connectionRequestPlayerIdSignal};

void testTask()
{
  Serial.println("Sending connection request");
}

void setup()
{
  g_canBus.Initialize(ICAN::BaudRate::kBaud1M);

  g_timerGroup.AddTimer(100, testTask);

  Serial.begin(9600);
  Serial.println("Started");
}

void loop() { g_timerGroup.Tick(millis()); }