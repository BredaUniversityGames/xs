#pragma once
#include <string>

namespace xs::profiler
{
	class ProfilerSection
	{
	public:
		ProfilerSection(const std::string& name);
		~ProfilerSection();
	private:
		std::string m_name;
	};

	void BeginSection(const std::string& name);
	void EndSection(const std::string& name);
	void Inspect(bool& show);
}
