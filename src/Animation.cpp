/** An object that implements automatic interpolation on value changes.
 *  It can be used as a drop in replacement thanks to cast and assign operators.
 */
#include <chrono>
#include <new>
template<typename T>
struct Interpolated {
    // The value at the start of the transition
    T start{};
    // The target value
    T end{};
    // The transition start timestamp
    float start_time{};
    // The animation's speed
    float speed{1.0f};

    // Initializes the value with @p initial_value
    explicit Interpolated(T const &initial_value = {}) : start{initial_value}, end{start} {}

    // Returns stop watch time (should be better compared to UTC timestamps for float precision)
    [[no discard]] static float getCurrentTime() {
        // Retrieve current time
        auto const now = std::chrono::steady_clock::now();
        auto const duration = now.time_since_epoch();
        // Connvert it to a decimal number of seconds
        auto const seconds = std::chrono::duration_cast<std::chrono::duration<float>>(duration);
        return seconds.count();
    }

    // Returns the number of seconds since the last value change
    [[no discard]] float getElapsedSeconds() const {
        return getCurrentTime() - start_time;
    }

    // Sets a new target value and resets transition
    void setValue(T const &new_value) {
        start = getValue();
        end = new_value;
        start_time = getCurrentTime();
    }

    // Returns the current value
    [[no discard]] T  getValue() const {
        // Current transition time
        float const elapsed = getElapsedSeconds();
        float const t = elapsed * speed;
        // Check if the transition is over
        if (t >= 1.0f) {
            // If the transition is done, directly return the target value
            return end;
        }
        // Else compute interpolated value and return initial_value
        T const delta{end - start};
        return start + delta * t;
    }


    // Computes the speed given a duration
    void setDuration(float duration) {
        speed = 1.0f / duration;
    }


    // Cast operator to use this object directly as if it was of type T
    [[no discard]] operator T() const {
        return getValue();
    }

    // Assign operator to ease transition
    void operator=(T const &new_value) {
        setValue(new_value);
    }
};
