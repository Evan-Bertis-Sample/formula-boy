#ifndef __PLAYER_INPUT_H__
#define __PLAYER_INPUT_H__

// handles the parsing of the input from the controller

#include <Arduino.h>
#include <CAN.h>
#include <memory>
#include <array>
#include <string>

class PlayerInput
{
private:
    std::uint8_t _playerId;
    float _verticalAxis;
    float _horizontalAxis;
    float _rotationAxis;
    std::uint8_t _buttonBitmask;

public:
    enum AXIS
    {
        VERTICAL,
        HORIZONTAL,
        ROTATION
    };

    PlayerInput(std::uint8_t playerId) : _playerId(playerId) {}
    std::uint8_t getPlayerId() { return _playerId; }

    void setAxis(AXIS axis, float value)
    {
        switch (axis)
        {
        case AXIS::VERTICAL:
            _verticalAxis = value;
            break;
        case AXIS::HORIZONTAL:
            _horizontalAxis = value;
            break;
        case AXIS::ROTATION:
            _rotationAxis = value;
            break;
        }
    }

    void setButton(std::uint8_t buttonBitmask)
    {
        _buttonBitmask = buttonBitmask;
    }

    std::string encodeInput()
    {
        std::string inputString = "Player ID: " + std::to_string(_playerId) + ",";
        inputString += "Vertical Axis: " + std::to_string(_verticalAxis) + ",";
        inputString += "Horizontal Axis: " + std::to_string(_horizontalAxis) + ",";
        inputString += "Rotation Axis: " + std::to_string(_rotationAxis) + ",";
        inputString += "Button Bitmask: " + std::to_string(_buttonBitmask) + "\n";
        return inputString;
    }
};

class InputHandler
{
public:
    InputHandler(CAN &canBus, VirtualTimerGroup &timerGroup) : _canBus(canBus), _timerGroup(timerGroup) {}

    void initialize()
    {
        _canBus.RegisterRXMessage(_controllerInputMessage);
    }

    void readInput()
    {
        // read the input from the controller
        std::uint8_t playerID = _playerIdSignal;

        if (playerID >= _MAX_PLAYERS || playerID < 0)
        {
            Serial.println("Invalid player ID");
            return;
        }

        if (_playerInputs[playerID] == nullptr)
        {
            Serial.println("Player not connected");
            return;
        }

        _playerInputs[playerID]->setAxis(PlayerInput::AXIS::VERTICAL, _verticalAxisSignal);
        _playerInputs[playerID]->setAxis(PlayerInput::AXIS::HORIZONTAL, _horizontalAxisSignal);
        _playerInputs[playerID]->setAxis(PlayerInput::AXIS::ROTATION, _rotationAxisSignal);
        _playerInputs[playerID]->setButton(_buttonBitmaskSignal);

        _timeSinceLastInput[playerID] = millis();
    }

    void connectPlayer(std::int8_t playerID)
    {
        if (playerID >= _MAX_PLAYERS || playerID < 0)
        {
            Serial.println("Invalid player ID");
            return;
        }

        if (_playerInputs[playerID] != nullptr)
        {
            Serial.println("Player already connected");
            return;
        }

        _playerInputs[playerID] = std::make_shared<PlayerInput>(playerID);
    }

    void disconnectPlayer(std::int8_t playerID)
    {
        if (playerID >= _MAX_PLAYERS || playerID < 0)
        {
            Serial.println("Invalid player ID");
            return;
        }

        if (_playerInputs[playerID] == nullptr)
        {
            Serial.println("Player not connected");
            return;
        }

        _playerInputs[playerID] = nullptr;
    }

    std::uint8_t getNumPlayers() const
    {
        std::uint8_t numPlayers = 0;
        for (auto &player : _playerInputs)
        {
            if (player != nullptr)
            {
                numPlayers++;
            }
        }
        return numPlayers;
    }

    std::vector<int> getConnectedPlayers()
    {
        std::vector<int> connectedPlayers;
        for (int i = 0; i < _MAX_PLAYERS; i++)
        {
            if (_playerInputs[i] != nullptr)
            {
                connectedPlayers.push_back(i);
            }
        }
        return connectedPlayers;
    }

    void sendInput()
    {
        for (auto &player : _playerInputs)
        {
            if (player != nullptr)
            {
                std::string inputString = player->encodeInput();
                Serial.println(inputString.c_str());
            }
        }
    }

    std::int8_t getNextPlayerId() const
    {
        for (std::int8_t i = 0; i < _MAX_PLAYERS; i++)
        {
            if (_playerInputs[i] == nullptr)
            {
                return i;
            }
        }
        return -1;
    }

    void tick()
    {
        // check for inactivity
        for (std::int8_t i = 0; i < _MAX_PLAYERS; i++)
        {
            if (_playerInputs[i] != nullptr)
            {
                unsigned long currentTime = millis();
                unsigned long timeSinceLastInput = currentTime - _timeSinceLastInput[i];
                if (timeSinceLastInput > _MAX_INACTIVITY)
                {
                    Serial.printf("Player %d has been inactive for too long\n", i);
                    disconnectPlayer(i);
                }
            }
        }
    }

private:
    static const std::uint32_t _CONTROLLER_INPUT_ADDRESS = 0x200;
    static const std::int8_t _MAX_PLAYERS = 3;
    static const std::uint8_t _MAX_INACTIVITY = 1000U;

    CAN &_canBus;
    VirtualTimerGroup &_timerGroup;
    std::array<std::shared_ptr<PlayerInput>, _MAX_PLAYERS> _playerInputs;
    std::array<unsigned long, _MAX_PLAYERS> _timeSinceLastInput;

    // CAN Signals for Player Input
    MakeSignedCANSignal(int8_t, 0, 8, 1, 0) _playerIdSignal{};           // one byte
    MakeSignedCANSignal(float, 8, 16, 0.01, 0) _verticalAxisSignal{};    // two bytes
    MakeSignedCANSignal(float, 24, 16, 0.01, 0) _horizontalAxisSignal{}; // two bytes
    MakeSignedCANSignal(float, 40, 16, 0.01, 0) _rotationAxisSignal{};   // two bytes
    MakeUnsignedCANSignal(uint8_t, 56, 8, 1, 0) _buttonBitmaskSignal{};  // one byte

    // CAN message for Player Input
    CANRXMessage<5> _controllerInputMessage{_canBus,
                                            _CONTROLLER_INPUT_ADDRESS,
                                            [this]()
                                            { this->readInput(); },
                                            _playerIdSignal,
                                            _verticalAxisSignal,
                                            _horizontalAxisSignal,
                                            _rotationAxisSignal,
                                            _buttonBitmaskSignal};

    // CAN message for input denial
};

#endif // __PLAYER_INPUT_H__