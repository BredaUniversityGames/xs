#include "xs.hpp"

#if defined(PLATFORM_PC) || defined(PLATFORM_LINUX)

#if defined(PLATFORM_WINDOWS) || defined(_WIN32)
#define DWORD unsigned int
extern "C"
{
    // Force Nvidia GPU if available
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

int main(int argc, char* argv[])
{
	return xs::dispatch(argc, argv);
}
#endif
