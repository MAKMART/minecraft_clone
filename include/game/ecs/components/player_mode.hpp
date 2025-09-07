#pragma once

enum Type { SURVIVAL,
	    CREATIVE,
	    SPECTATOR };

struct PlayerMode {
	explicit PlayerMode(Type starting_mode) : mode(starting_mode)
	{
	}
	PlayerMode() = default;
	Type mode    = SURVIVAL;
};
