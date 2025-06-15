// RunningState.h
#pragma once
#include "../Player.h" // Include full Player definition
#include "PlayerState.h"

class RunningState : public PlayerState {
  public:
    void enterState(Player &player) override;

    void exitState(Player &player) override;

    void handleInput(Player &player, float deltaTime) override;
};
