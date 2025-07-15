// SurvivalMode.h
#pragma once
#include "PlayerMode.h"
#include "../player_states/WalkingState.h"
#include "../player_states/RunningState.h"
#include "../player_states/SwimmingState.h"

class SurvivalMode : public PlayerMode {
  public:
    void enterMode(Player &player) override {
        player.isSurvival = true;
        player.hasHealth = true;
        player.hasInfiniteBlocks = false;
        player.isDamageable = true;
        player.canBreakBlocks = true;
        player.canPlaceBlocks = true;
        player.renderSkin = true;

    }

    void exitMode(Player &player) override {
        player.isSurvival = false;
    }

    void updateState(Player &player, std::unique_ptr<PlayerState> newState) override {
        // Only allow the player to be walking || running || swimming in survival mode
        if (player.isWalking || player.isRunning || player.isSwimming) {
            player.changeState(std::move(newState)); // Move ownership
        }
    }
};
