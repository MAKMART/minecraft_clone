/*
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <deque>
*/
module;
#include <cuchar>
export module timer;

import std;

constexpr size_t MaxTimingSamples = 100;

/*struct TimingHistory {
    std::vector<float> samples = std::vector<float>(MaxTimingSamples, 0.0f);
    size_t index = 0;

    void addSample(float value) {
        samples[index] = value;
        index = (index + 1) % MaxTimingSamples;
    }

    float latest() const {
        return samples[(index - 1 + MaxTimingSamples) % MaxTimingSamples];
    }
};*/
constexpr size_t MaxHistoryFrames = 500;

struct TimingSeries {
	std::deque<float> values;
};

struct TimingHistory {
	std::mutex                                    mutex;
	std::unordered_map<std::string, TimingSeries> timings;
};

extern TimingHistory g_TimingHistory;

struct ImGuiTimerData {
	std::mutex                                     mutex;
	std::unordered_map<std::string, TimingHistory> timings;
};

extern ImGuiTimerData g_TimerData;

// Constants
extern const int          frametime_max; // Store last 100 frame times
extern std::vector<float> frametimes;    // Declare without initialization here
int frameIndex = 0;

export class Timer
{
      public:
	Timer(const std::string& name);
	~Timer();

      private:
	std::string                                    name;
	std::chrono::steady_clock::time_point start;
};

// Call this every frame
export void UpdateFrametimeGraph(float deltaTime);
export void RenderTimings();
