#ifndef __CONNECTION_HANDLER_H__
#define __CONNECTION_HANDLER_H__

// handles the connection request and response for the controller bus

#include <Arduino.h>
#include <CAN.h>
#include <memory>
#include <map>

#include "player_input.hpp"

typedef CANSignal<int8_t, 0, 8, CANTemplateConvertFloat(1),
                  CANTemplateConvertFloat(0), true,
                  ICANSignal::ByteOrder::kLittleEndian>
    PlayerIDSignal;

class ConnectionHandler
{
public:
    ConnectionHandler(CAN &canBus, VirtualTimerGroup &timerGroup, std::shared_ptr<InputHandler> inputHandler) : _canBus(canBus), _timerGroup(timerGroup), _inputHandler(inputHandler) {}

    void initialize()
    {
        this->_canBus.RegisterRXMessage(this->_connectionRequestMessage);
        Serial.println("Connection Handler Initialized");
    }

    void requestCallback()
    {
        int8_t deviceId = this->_connectionRequestPlayerIdSignal;
        Serial.printf("Connection Request Received from Device %d\n", deviceId);

        // send a response with the player id
        int8_t playerNumber = this->_inputHandler->getNextPlayerId();
        if (playerNumber == -1)
        {
            // no more players can connect
            Serial.println("Connection Request Denied: No More Players Can Connect");
        }
        else
        {
            Serial.printf("Connection Request Accepted: Player %d\n", playerNumber);
            this->_inputHandler->connectPlayer(playerNumber);
        }


        // send the response
        // should be device id, player id
        this->_connectionRequestMessageData[0] = deviceId;
        this->_connectionRequestMessageData[1] = playerNumber;
        this->_canBus.SendMessage(this->_connectionResponseMessage);
    }

private:
    const uint32_t _CONTROLLER_CONNECTION_ADDRESS = 0x000;
    const uint32_t _CONTROLLER_CONNECTION_RESPONSE_ADDRESS = 0x100;

    CAN _canBus;
    VirtualTimerGroup _timerGroup;
    std::shared_ptr<InputHandler> _inputHandler;

    CANRXMessage<1> _connectionRequestMessage{_canBus, _CONTROLLER_CONNECTION_ADDRESS, [this](){this->requestCallback();}, _connectionRequestPlayerIdSignal};
    MakeSignedCANSignal(int8_t, 0, 8, 1, 0) _connectionRequestPlayerIdSignal{}; // one byte

    // CAN Signals for Connection Request/Acknowledgement
    std::array<uint8_t, 8> _connectionRequestMessageData{0};
    CANMessage _connectionResponseMessage{
        _CONTROLLER_CONNECTION_RESPONSE_ADDRESS, 2, _connectionRequestMessageData}; // two bytes -- random player id, actual player id

};

#endif // __CONNECTION_HANDLER_H__