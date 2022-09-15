#include <cstdio>
#include "fileio.h"
#include "device.h"
#include "input.h"
#include "log.h"
#include "render.h"
#include "script.h"
#include "audio.h"
#include "account.h"
#include "data.h"
#include "inspector.h"
#include "version.h"
#include <glm/glm.hpp>
#include <chrono>

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
#include <version.h>
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

#endif


int xs::main(int argc, char* argv[])
{	
	log::initialize();
    log::info("  __ __ _____ ");
    log::info(" |  |  |   __|");
    log::info(" |-   -|__   |");
    log::info(" |__|__|_____| " + xs::version::version_string);
    log::info("Made with love at Breda University of Applied Sciences");
    log::info("");


    string main_script;

    // TODO: This approach fails on consoles
    if (argc == 2)
    {       
        main_script = string(argv[1]);        
    }
    else
    {
        log::info("No arguments provided script to run. Trying games/init.txt");
        if (fileio::exists("games/init.txt"))
            main_script = fileio::read_text_file("games/init.txt");
    }

    if(main_script.empty())
    {
        log::info("Please provide a game script to run. Example:");
        log::info("xs.exe [games]/awesome_game/main.wren");
        log::info("Or provide an init.txt file with the relative path to the script.");
        return -1;
    }
    
    account::initialize();
    fileio::initialize(main_script);
    data::initialize();
    script::configure(main_script);
    device::initialize();
    render::initialize();
    input::initialize();
    audio::initialize();
    inspector::initialize();
    script::initialize();

    auto prev_time = chrono::high_resolution_clock::now();
    while (!device::should_close())
	{
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = current_time - prev_time;
        prev_time = current_time;
        const auto dt = std::chrono::duration<double>(elapsed).count();

        device::poll_events();
        input::update(dt);
        if (!inspector::paused())
        {
            render::clear();
            script::update(dt);
            script::render();
        }
        render::render();
        inspector::render(float(dt));
        device::swap_buffers();
	}

    inspector::shutdown();
    audio::shutdown();
    input::shutdown();
    render::shutdown();
    device::shutdown();
    script::shutdown();
    account::shutdown();
    data::shutdown();

	return 0;
}

