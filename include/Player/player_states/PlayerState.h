// PlayerState.h
#pragma once

class Player;
class PlayerState {
public:
    virtual ~PlayerState() = default;
    
    virtual void enterState(Player& player) = 0;
    virtual void exitState(Player& player) = 0;
    virtual void handleInput(Player& player, float deltaTime) = 0;
};
