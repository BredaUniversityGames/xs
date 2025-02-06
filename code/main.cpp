#include "xs.hpp"
#include "device.hpp"
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

using namespace std;

namespace xs{ int main(int argc, char* argv[]); }

#if defined(PLATFORM_PC)
#define DWORD unsigned int
extern "C"
{
    // Force Nvidia GPU if available
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

#if defined(PLATFORM_SWITCH)

#include <nn/os.h>
#include <version.h>
#include <glm/gtc/type_ptr.hpp>
extern "C" void nnMain()
{
	const auto argc = nn::os::GetHostArgc();
	const auto argv = nn::os::GetHostArgv();
	xs::main(argc, argv);
}

#elif defined(PLATFORM_PC) 
int main(int argc, char* argv[])
{
	return  xs::main(argc, argv);	
}

#elif defined(PLATFORM_PS5) 
int main(int argc, char* argv[])
{
    return  xs::main(argc, argv);
}

#elif defined(PLATFORM_APPLE)
int main(int argc, char* argv[])
{
    static_assert(false);
}
#endif


int xs::main(int argc, char* argv[])
{	
	xs::initialize();
    auto prev_time = chrono::high_resolution_clock::now();
    while (!device::should_close())
    {
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = current_time - prev_time;
        prev_time = current_time;
        auto dt = std::chrono::duration<double>(elapsed).count();
        if (dt > 0.03333) dt = 0.03333;
		xs::update((float)dt);
    }
	xs::shutdown();
	return 0;
}
