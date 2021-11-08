#include <cstdio>
#include <GLFW/glfw3.h>
#include "fileio.h"
#include "device.h"
#include "input.h"
#include "log.h"
#include "render.h"
#include "script.h"
#include "account.h"
#include <glm/glm.hpp>

#include "inspector.h"

using namespace std;

namespace xs { int main(int argc, char* argv[]); }

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
#endif


int xs::main(int argc, char* argv[])
{	
	log::initialize();
    log::info("  __ __ _____ ");
    log::info(" |  |  |   __|");
    log::info(" |-   -|__   |");
    log::info(" |__|__|_____| v0.13");
    log::info("By Bojan Endrovski & Friends");
    log::info("");	    

    if (argc < 2)
    {
        log::info("Please provide a game script to run. Example:");
        log::info("xs.exe [games]/awesome_game/main.wren");
        return -1;
    }

    const auto main = argv[1];
    account::initialize();
    fileio::initialize();    
    script::initialize(main);
    device::initialize();
    render::initialize();
    input::initialize();
    inspector::initialize();

    auto prev_time = chrono::high_resolution_clock::now();
    while (!device::should_close())
	{
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = current_time - prev_time;
        prev_time = current_time;
        const auto dt = std::chrono::duration<double>(elapsed).count();

        device::poll_events();
        input::update();
        if (!inspector::paused())
        {
            render::clear();
            script::update(dt);
            render::render();
        }
        inspector::render();
        device::swap_buffers();
	}

    inspector::shutdown();
    input::shutdown();
    render::shutdown();
    device::shutdown();
    script::shutdown();
    account::shutdown();

	return 0;
}

