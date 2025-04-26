// SpectatorMode.h
#pragma once
#include "PlayerMode.h"
#include "../player_states/FlyingState.h"

class SpectatorMode : public PlayerMode {
public:
    void enterMode(Player& player) override {
        player.isDamageable = false;  // No damage
        player.hasInfiniteBlocks = false;
        player.isFlying = true;
	player.canBreakBlocks = false;
	player.canPlaceBlocks = false;
	player.renderSkin = false;
        player.changeState(std::make_unique<FlyingState>()); // Spectators always fly
    }

    void exitMode(Player& player) override {
	player.renderSkin = true;
    }

    void updateState(Player& player, std::unique_ptr<PlayerState> newState) override {
        // In Spectator mode, only allow Flying
	PlayerState* rawPtr = newState.get();
        if (dynamic_cast<FlyingState*>(rawPtr)) {
            player.changeState(std::move(newState));
        }
    }
};

