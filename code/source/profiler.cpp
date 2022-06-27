#include "profiler.h"
#include <unordered_map>
#include <deque>
#include <chrono>
#include <vector>
#include <imgui/imgui.h>
#include <imgui/implot.h>

namespace xs::profiler::internal
{

using time_t = std::chrono::time_point<std::chrono::steady_clock>;
using span_t = std::chrono::nanoseconds;

struct entry
{
	time_t start{};
	time_t end{};
	span_t accum{};
	float avg = 0.0f;
	std::deque<float> history;
};
std::unordered_map<std::string, entry> times;

}

using namespace xs::profiler;
using namespace xs::profiler::internal;

profiler_section::profiler_section(const std::string& name) : m_name(name)
{
    begin_section(m_name);
}

profiler_section::~profiler_section()
{
    end_section(m_name);
}

void xs::profiler::begin_section(const std::string& name)
{
    //times[name].start = std::chrono::high_resolution_clock::now();
    // auto start = std::chrono::high_resolution_clock::now();
}

void xs::profiler::end_section(const std::string& name)
{
    // auto end = std::chrono::high_resolution_clock::now();

    /*
    auto& e = times[name];
    e.end = std::chrono::high_resolution_clock::now();
    auto elapsed = e.end - e.start;
    e.accum += elapsed;
    */
}

void xs::profiler::inspect(bool& show)
{
    ImGui::Begin("Profiler", &show, ImGuiWindowFlags_NoCollapse);

    for (auto& itr : times)
    {
        auto& e = itr.second;
        float duration = (float)((double)e.accum.count() / 1000000.0);
        if (e.history.size() > 100)
            e.history.pop_front();
        e.history.push_back(duration);

        e.avg = 0.0f;
        for (float f : e.history)
            e.avg += f;

        e.avg /= (float)e.history.size();
    }

    if (ImPlot::BeginPlot("Profiler", ImVec2(-1, 200)))
    {
        ImPlot::SetupAxes("Sample", "Time");
        ImPlot::SetupAxesLimits(0, 50, 0, 20);
        for (auto& itr : times)
        {
            auto& e = itr.second;

            std::vector<float> vals(
                e.history.begin(),
                e.history.end());

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded(itr.first.c_str(), vals.data(), (int)vals.size());
            ImPlot::PopStyleVar();
            ImPlot::PlotLine(itr.first.c_str(), vals.data(), (int)vals.size());
        }
        ImPlot::EndPlot();
    }

    for (auto& itr : times)
        ImGui::LabelText(itr.first.c_str(), "%f ms", itr.second.avg);

    ImGui::End();

    for (auto& itr : times)
        itr.second.accum = {};
}


