// SwimmingState.h

#pragma once
#include "../Player.h"
#include "PlayerState.h"

class SwimmingState : public PlayerState {
  public:
    void enterState(Player &player) override;

    void exitState(Player &player) override;

    void handleInput(Player &player, float deltaTime) override;
};
