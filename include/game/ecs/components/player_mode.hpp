#pragma once

enum Type { SURVIVAL,
	    CREATIVE,
	    SPECTATOR };

struct PlayerMode {
	PlayerMode(Type starting_mode) : mode(starting_mode)
	{
	}
	PlayerMode() = default;
	Type mode    = SURVIVAL;
};
