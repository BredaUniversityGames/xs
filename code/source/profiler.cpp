#include "profiler.h"
#include <unordered_map>
#include <deque>
#include <chrono>
#include <vector>
#include <imgui/imgui.h>
#include <imgui/implot.h>

namespace xs::profiler::internal
{

using TimeT = std::chrono::time_point<std::chrono::steady_clock>;
using SpanT = std::chrono::nanoseconds;

struct Entry
{
	TimeT Start{};
	TimeT End{};
	SpanT Accum{};
	float Avg = 0.0f;
	std::deque<float> History;
};
std::unordered_map<std::string, Entry> m_times;

}

using namespace xs::profiler;
using namespace xs::profiler::internal;

ProfilerSection::ProfilerSection(const std::string& name) : m_name(name)
{
    BeginSection(m_name);
}

ProfilerSection::~ProfilerSection()
{
    EndSection(m_name);
}

void xs::profiler::BeginSection(const std::string& name)
{
    m_times[name].Start = std::chrono::high_resolution_clock::now();
}

void xs::profiler::EndSection(const std::string& name)
{
    auto& e = m_times[name];
    e.End = std::chrono::high_resolution_clock::now();
    auto elapsed = e.End - e.Start;
    e.Accum += elapsed;
}

void xs::profiler::Inspect(bool& show)
{
    ImGui::Begin("Profiler", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoCollapse);

    for (auto& itr : m_times)
    {
        auto& e = itr.second;
        float duration = (float)((double)e.Accum.count() / 1000000.0);
        if (e.History.size() > 100)
            e.History.pop_front();
        e.History.push_back(duration);

        e.Avg = 0.0f;
        for (float f : e.History)
            e.Avg += f;

        e.Avg /= (float)e.History.size();
    }

    if (ImPlot::BeginPlot("Profiler"))
    {
        ImPlot::SetupAxes("Sample", "Time");
        ImPlot::SetupAxesLimits(0, 100, 0, 20);
        for (auto& itr : m_times)
        {
            auto& e = itr.second;

            std::vector<float> vals(
                e.History.begin(),
                e.History.end());

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded(itr.first.c_str(), vals.data(), (int)vals.size());
            ImPlot::PopStyleVar();
            ImPlot::PlotLine(itr.first.c_str(), vals.data(), (int)vals.size());
        }
        ImPlot::EndPlot();
    }

    for (auto& itr : m_times)
        ImGui::LabelText(itr.first.c_str(), "%f ms", itr.second.Avg);

    ImGui::End();

    for (auto& itr : m_times)
        itr.second.Accum = {};
}


