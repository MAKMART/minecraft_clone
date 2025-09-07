#pragma once

struct Health {
	int current = 20;
	int max     = 20;

	Health() = default;
	explicit Health(int maxHP) : current(maxHP), max(maxHP)
	{
	}
};
