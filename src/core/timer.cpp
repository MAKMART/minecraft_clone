#include "core/timer.hpp"
#include <chrono>
#include <imgui.h>
#include "core/logger.hpp"

TimingHistory  g_TimingHistory;
ImGuiTimerData g_TimerData;

const int frametime_max = 100;
std::vector<float> frametimes(frametime_max, 0.0f);
int frameIndex = 0;

Timer::Timer(const std::string& name) : name(name), start(std::chrono::high_resolution_clock::now()) {}

Timer::~Timer()
{
	using namespace std::chrono;
	auto  end = high_resolution_clock::now();
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
	frameIndex = (frameIndex + 1) % frametime_max;

	std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);
	auto& frameSeries = g_TimingHistory.timings["FrameTime"].values;
	if (frameSeries.size() >= MaxHistoryFrames) {
		frameSeries.pop_front();
	}
	frameSeries.emplace_back(deltaTime);
}
void RenderTimings()
{
	//static int frameSkip = 0;
	//if (++frameSkip % 2 != 0) return;
    std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);
	if (g_TimingHistory.timings.empty()) return;

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

    int colorIndex = 0;

    // Cache latest values + draw legend in one pass
    struct LegendEntry { const char* label; ImU32 color; float latestMs; };
    std::vector<LegendEntry> legend;
    legend.reserve(g_TimingHistory.timings.size());

	for (const auto& [label, series] : g_TimingHistory.timings)
    {
        const auto& values = series.values;
        size_t count = values.size();
        if (count < 2) continue;

        // Calculate color...
        ImU32 colorU32 = ImColor(ImColor::HSV(colorIndex++ * 0.618f, 0.7f, 0.9f));

        drawList->PathClear();
        for (size_t i = 0; i < count; ++i)
        {
            // Calculate X on the fly to avoid static vector mismatch
            float x = origin.x + (float(i) / float(count - 1)) * graphSize.x;
            float y = origin.y + graphSize.y * (1.0f - std::min(values[i], maxMs) * invMaxMs);
            drawList->PathLineTo(ImVec2(x, y));
        }
        drawList->PathStroke(colorU32, false, 1.5f);
        
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
