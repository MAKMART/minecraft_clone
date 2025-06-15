// PlayerMode.h
#pragma once
#include "../player_states/PlayerState.h"
#include <memory>

class Player;
class PlayerMode {
  public:
    virtual ~PlayerMode() = default;

    virtual void enterMode(Player &player) = 0;
    virtual void exitMode(Player &player) = 0;
    virtual void updateState(Player &player, std::unique_ptr<PlayerState> newState) = 0;
};
