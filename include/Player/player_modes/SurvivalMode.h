// SurvivalMode.h
#pragma once
#include "PlayerMode.h"
#include "../player_states/WalkingState.h"
#include "../player_states/RunningState.h"
#include "../player_states/SwimmingState.h"

class SurvivalMode : public PlayerMode {
  public:
    void enterMode(Player &player) override {
        player.hasHealth = true;
        player.hasInfiniteBlocks = false;
        player.isDamageable = true;
        player.canBreakBlocks = true;
        player.canPlaceBlocks = true;
        player.renderSkin = true;

        player.changeState(std::make_unique<WalkingState>()); // Default state in Survival is Walking
    }

    void exitMode(Player &player) override {
        (void)player;
        // Cleanup if necessary
    }

    void updateState(Player &player, std::unique_ptr<PlayerState> newState) override {
        // Extract raw pointer for dynamic_cast
        PlayerState *rawPtr = newState.get();

        if (dynamic_cast<WalkingState *>(rawPtr) ||
            dynamic_cast<RunningState *>(rawPtr) ||
            dynamic_cast<SwimmingState *>(rawPtr)) {
            player.changeState(std::move(newState)); // Move ownership
        }
    }
};
