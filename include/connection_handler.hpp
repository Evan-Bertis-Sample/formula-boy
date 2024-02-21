#ifndef __CONNECTION_HANDLER_H__
#define __CONNECTION_HANDLER_H__

// handles the connection request and response for the controller bus

#include <Arduino.h>
#include <CAN.h>
#include <memory>

#include "player_input.hpp"

typedef CANSignal<int8_t, 0, 8, CANTemplateConvertFloat(1),
                  CANTemplateConvertFloat(0), true,
                  ICANSignal::ByteOrder::kLittleEndian>
    PlayerIDSignal;

class ConnectionHandler
{
public:
    enum ConnectionMessageType
    {
        CONNECTION_REQUEST = 0,
        CONNECTION_ACKNOWLEDGEMENT = 1
    };

    enum ConnectionResponseType
    {
        CONNECTION_ACCEPTED = 0,
        CONNECTION_REJECTED = 1
    };

    struct PlayerConnectionResponse
    {
        std::shared_ptr<PlayerInput> playerInput;
        std::string message;
        ConnectionResponseType response;

        static PlayerConnectionResponse success(std::shared_ptr<PlayerInput> playerInput)
        {
            PlayerConnectionResponse response;
            response.playerInput = playerInput;
            response.message = "Connection Accepted";
            response.response = ConnectionResponseType::CONNECTION_ACCEPTED;
            return response;
        }

        static PlayerConnectionResponse failure(std::string message)
        {
            PlayerConnectionResponse response;
            response.message = message;
            response.response = ConnectionResponseType::CONNECTION_REJECTED;
            return response;
        }

    private:
        PlayerConnectionResponse() = default;
    };

    ConnectionHandler(CAN &canBus, VirtualTimerGroup &timerGroup) : _canBus(canBus), _timerGroup(timerGroup) {}

    void initialize()
    {
        this->_canBus.RegisterRXMessage(this->_connectionRequestMessage);
    }

    void requestCallback()
    {
    }

    PlayerConnectionResponse handleConnectionRequest()
    {
    }

private:
    const uint32_t _CONTROLLER_CONNECTION_ADDRESS = 0x000;
    const uint32_t _CONTROLLER_CONNECTION_RESPONSE_ADDRESS = 0x100;

    CAN _canBus;
    VirtualTimerGroup _timerGroup;

    CANRXMessage<1> _connectionRequestMessage{_canBus, _CONTROLLER_CONNECTION_ADDRESS, _connectionRequestPlayerIdSignal};
    CANMessage _connectionResponseMessage{
        _CONTROLLER_CONNECTION_RESPONSE_ADDRESS, 8, std::array<uint8_t, 8>{}};

    // CAN Signals for Connection Request/Acknowledgement
    MakeSignedCANSignal(int8_t, 0, 8, 1, 0) _connectionRequestPlayerIdSignal{}; // one byte
};

#endif // __CONNECTION_HANDLER_H__