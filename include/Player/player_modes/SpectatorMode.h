// SpectatorMode.h
#pragma once
#include "PlayerMode.h"
#include "../player_states/FlyingState.h"

class SpectatorMode : public PlayerMode {
  public:
    void enterMode(Player &player) override {
        player.isSpectator = true;
        player.isDamageable = false; // No damage
        player.hasInfiniteBlocks = false;
        player.isFlying = true;
        player.canBreakBlocks = false;
        player.canPlaceBlocks = false;
        player.renderSkin = false;
        player.changeState<FlyingState>(); // Spectators always fly
    }

    void exitMode(Player &player) override {
        player.isSpectator = false;
        player.renderSkin = true;
    }

    void updateState(Player &player, std::unique_ptr<PlayerState> newState) override {
        // Only allow the player to be flying in spectator mode
        if (player.isFlying) {
            player.changeState(std::move(newState));
        }
    }
};
