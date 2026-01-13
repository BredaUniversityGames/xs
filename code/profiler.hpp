#pragma once
#include <string>

#define XS_PROFILE_FUNCTION() xs::profiler::profiler_section s_sect(__FUNCTION__)
#define XS_PROFILE_SECTION(id) xs::profiler::profiler_section s_sect(id)

namespace xs::profiler
{
	class profiler_section
	{
	public:
		profiler_section(const std::string& name);
		~profiler_section();
	private:
		std::string m_name;
	};

	void begin_section(const std::string& name);
	void end_section(const std::string& name);
	void begin_timing();
	double end_timing();
	void inspect();
}
