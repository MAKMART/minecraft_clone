#include "core/timer.hpp"
#include <chrono>
#include <imgui.h>
#include "core/logger.hpp"

TimingHistory  g_TimingHistory;
ImGuiTimerData g_TimerData;

const int          frametime_max = 100;
std::vector<float> frametimes(frametime_max, 0.0f);
int                frameIndex = 0;

Timer::Timer(const std::string& name)
    : name(name), start(std::chrono::high_resolution_clock::now())
{
}

Timer::~Timer()
{
	using namespace std::chrono;
	auto  end        = high_resolution_clock::now();
	float durationMs = duration<float, std::milli>(end - start).count();

	std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);
	auto&                       series = g_TimingHistory.timings[name].values;

	if (series.size() >= MaxHistoryFrames) {
		series.pop_front();
	}
	series.push_back(durationMs);
}
void UpdateFrametimeGraph(float deltaTime)
{
	frametimes[frameIndex] = deltaTime;
	frameIndex             = (frameIndex + 1) % frametime_max;

	std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);
	auto&                       frameSeries = g_TimingHistory.timings["FrameTime"].values;
	if (frameSeries.size() >= MaxHistoryFrames) {
		frameSeries.pop_front();
	}
	frameSeries.push_back(deltaTime);
}
void RenderTimings()
{
	//static int frameSkip = 0;
	//if (++frameSkip % 2 != 0) return;
    std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);

    const float graphHeight = 100.0f;
    const float graphWidth  = ImGui::GetContentRegionAvail().x;
    ImGui::Text("Timing History");

    ImVec2 graphSize = ImVec2(graphWidth, graphHeight);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin    = ImGui::GetCursorScreenPos();

    // Reserve space once
    ImGui::Dummy(graphSize);

    const float maxMs          = 15.0f;
    const float invMaxMs       = 1.0f / maxMs;
    const ImVec2 bottomRight   = ImVec2(origin.x + graphSize.x, origin.y + graphSize.y);

    // Light background rectangle
    drawList->AddRectFilled(origin, bottomRight, IM_COL32(20, 20, 30, 160));
    drawList->AddRect(origin, bottomRight, IM_COL32(80, 80, 100, 100));

    // Precompute x positions once (shared across all series)
    static std::vector<float> xPositions;
    const size_t histSize = g_TimingHistory.timings.empty() ? 0 : g_TimingHistory.timings.begin()->second.values.size();
    if (xPositions.size() != histSize)
    {
        xPositions.resize(histSize);
        for (size_t i = 0; i < histSize; ++i)
            xPositions[i] = origin.x + (float(i) / float(histSize - 1)) * graphSize.x;
    }

    int colorIndex = 0;

    // Cache latest values + draw legend in one pass
    struct LegendEntry { const char* label; ImU32 color; float latestMs; };
    std::vector<LegendEntry> legend;
    legend.reserve(g_TimingHistory.timings.size());

    for (const auto& [label, series] : g_TimingHistory.timings)
    {
        if (series.values.size() < 2) continue;

        const float hue   = (colorIndex++ * 0.61803398875f); // golden ratio for nicer color distribution
        ImVec4 col        = ImColor::HSV(hue, 0.7f, 0.9f);
        ImU32 colorU32    = ImColor(col);

        const auto& values = series.values;
        float prevY = origin.y + graphSize.y * (1.0f - std::min(values[0], maxMs) * invMaxMs);

        // Draw polyline instead of individual lines — much faster
        drawList->PathClear();
        drawList->PathLineTo(ImVec2(xPositions[0], prevY));

        for (size_t i = 1; i < values.size(); ++i)
        {
            float y = origin.y + graphSize.y * (1.0f - std::min(values[i], maxMs) * invMaxMs);
            drawList->PathLineTo(ImVec2(xPositions[i], y));
            prevY = y;
        }

        drawList->PathStroke(colorU32, false, 1.5f);

        // Store for legend
        legend.push_back({label.c_str(), colorU32, values.back()});
    }

    // Draw legend in a single column (or wrap if too many)
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Latest timings:");

    for (const auto& entry : legend)
    {
        ImGui::ColorButton("##color", ImColor(entry.color), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder, ImVec2(12, 12));
        ImGui::SameLine();
        ImGui::Text("%.2f ms  %s", entry.latestMs, entry.label);
    }
}
