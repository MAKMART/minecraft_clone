#pragma once
#include "../Player.h" // Include full Player definition
#include "PlayerState.h"

class WalkingState : public PlayerState {
  public:
    void enterState(Player &player) override;
    void exitState(Player &player) override;
    void handleInput(Player &player, float deltaTime) override;
};
