#include <iostream>
#include <chrono>

struct Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::chrono::duration<float> duration;
    const char *name;

    Timer(const char *_name) : name(_name) {
        start = std::chrono::high_resolution_clock::now();
    }

    ~Timer() {
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        float ms = duration.count() * 1000.0f; // Convert seconds to milliseconds
        std::cout << name << " took: " << ms << " ms\n";
    }
};
