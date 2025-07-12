#include "Timer.h"
#include <chrono>
#include <imgui.h>
#include "logger.hpp"

TimingHistory g_TimingHistory;
ImGuiTimerData g_TimerData;

const int frametime_max = 100;
std::vector<float> frametimes(frametime_max, 0.0f);
int frameIndex = 0;

Timer::Timer(const std::string& name)
    : name(name), start(std::chrono::high_resolution_clock::now()) {}

Timer::~Timer() {
    using namespace std::chrono;
    auto end = high_resolution_clock::now();
    float durationMs = duration<float, std::milli>(end - start).count();

    std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);
    auto& series = g_TimingHistory.timings[name].values;

    if (series.size() >= MaxHistoryFrames) {
        series.pop_front();
    }
    series.push_back(durationMs);
}
void UpdateFrametimeGraph(float deltaTime) {
    frametimes[frameIndex] = deltaTime;
    frameIndex = (frameIndex + 1) % frametime_max;

    std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);
    auto& frameSeries = g_TimingHistory.timings["FrameTime"].values;
    if (frameSeries.size() >= MaxHistoryFrames) {
        frameSeries.pop_front();
    }
    frameSeries.push_back(deltaTime);
}
void RenderTimings() {
    std::lock_guard<std::mutex> lock(g_TimingHistory.mutex);

    const float graphHeight = 100.0f;
    const float graphWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Text("Timing History");
    ImVec2 graphSize = ImVec2(graphWidth, graphHeight);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(graphSize); // Reserve space

    const float maxMs = 100.0f;
    ImVec2 bottomRight = ImVec2(origin.x + graphSize.x, origin.y + graphSize.y);
    drawList->AddRect(origin, bottomRight, IM_COL32(255,255,255,64));


    int colorIndex = 0;
    for (const auto& [label, series] : g_TimingHistory.timings) {
        if (series.values.size() < 2) continue;

        float hue = (colorIndex++) / 10.0f;
        ImVec4 color = ImColor::HSV(hue, 0.7f, 0.9f);
        ImU32 colorU32 = ImColor(color);

        for (size_t i = 1; i < series.values.size(); ++i) {
            float x0 = origin.x + ((i - 1) / float(MaxHistoryFrames - 1)) * graphSize.x;
            float x1 = origin.x + (i / float(MaxHistoryFrames - 1)) * graphSize.x;

            float y0 = origin.y + graphSize.y * (1.0f - std::min(series.values[i - 1], maxMs) / maxMs);
            float y1 = origin.y + graphSize.y * (1.0f - std::min(series.values[i], maxMs) / maxMs);

            drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), colorU32, 1.5f);
        }

        float latestMs = series.values.back();
        ImGui::ColorButton(label.c_str(), color, ImGuiColorEditFlags_NoTooltip, ImVec2(10, 10));
        ImGui::SameLine();
        ImGui::Text("%s took: %.2f ms", label.c_str(), latestMs);
        //log::info("{} took: {} ms", label, latestMs);
    }
}
