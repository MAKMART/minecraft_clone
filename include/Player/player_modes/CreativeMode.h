// CreativeMode.h
#pragma once
#include "PlayerMode.h"
#include "../player_states/FlyingState.h"
#include "../player_states/WalkingState.h"
#include "../player_states/SwimmingState.h"
#include "../player_states/RunningState.h"

class CreativeMode : public PlayerMode {
  public:
    void enterMode(Player &player) override {
        player.isDamageable = false; // No damage
        player.hasInfiniteBlocks = true;
        player.canPlaceBlocks = true;
        player.canBreakBlocks = true;
    }

    void exitMode(Player &player) override {
        player.hasInfiniteBlocks = false;
        player.isDamageable = true;
    }

    void updateState(Player &player, std::unique_ptr<PlayerState> newState) override {
        // In Creative mode, allow all possible states
        player.changeState(std::move(newState));
    }
};
