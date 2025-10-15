#include "xs.hpp"
#include "device.hpp"
#include <chrono>
#include "main.hpp"

using namespace std;


#if defined(PLATFORM_PC)
#define DWORD unsigned int
extern "C"
{
    // Force Nvidia GPU if available
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main(int argc, char* argv[])
{
	return  xs::main(argc, argv);	
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
        dt = std::min(dt, 0.03333);
        xs::update((float)dt);
    }
	xs::shutdown();
	return 0;
}
