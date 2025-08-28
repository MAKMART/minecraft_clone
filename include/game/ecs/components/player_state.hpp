#pragma once

enum class PlayerMovementState {
	Idle,
	Walking,
	Running,
	Crouching,
	Jumping,
	Swimming,
	Flying,
	Falling,
};

struct PlayerState {
	PlayerMovementState current    = PlayerMovementState::Idle;
	PlayerMovementState previous   = PlayerMovementState::Idle;
	bool                isOnGround = true;
};
