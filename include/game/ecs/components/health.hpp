#pragma once

struct Health {
    int current = 20;
    int max = 20;

    Health() = default;
    Health(int maxHP) : current(maxHP), max(maxHP) {}
};
