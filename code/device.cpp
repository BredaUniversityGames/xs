#include "device.hpp"

namespace xs::device
{
	platform get_platform()
	{
#ifdef PLATFORM_PC 
		return platform::PC;
#endif
#ifdef PLATFORM_PS5
		return platform::PS5;
#endif
#ifdef PLATFORM_SWITCH 
		return platform::SWITCH;
#endif
		return platform::PLATFORM_UNKNOWN;
	}
}

