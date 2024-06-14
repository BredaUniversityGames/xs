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
#include "cooker.h"
#include "tools.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

// The type of execution the engine is used for
// 
// Parsing arguments
// Running a game
enum class EngineExecution
{
    DEFAULT,
    ARUGMENTS_PARSED,
    ARGUMENTS_PARSED_ERROR
};

EngineExecution argument_parser(int argc, char* argv[])
{
#if defined(PLATFORM_PC)
    if (argc == 2)
    {
        // First argument path of the executable
        // So starting from 1 already

        int p = 1;
        while (p < argc)
        {
            if (argv[p][0] == '-')
            {
                switch (argv[p][1])
                {
                case 'c':
                {
                    std::vector<std::string> split = xs::tools::string_split(argv[p], "=");

                    if (split.size() != 2)
                    {
                        xs::log::error("Unknown game to cook, cooking argument usage: -c=name_of_game");
                        return EngineExecution::ARGUMENTS_PARSED_ERROR;
                    }

                    xs::log::info("Start cooking content for ./games/{0}", "shared");
                    xs::log::info("Start cooking content for ./games/{0}", split[1]);

                    if (xs::cooker::cook_content("./games", { "shared", split[1] }) == false)
                    {
                        xs::log::error("Failed to cook content!");
                        return EngineExecution::ARGUMENTS_PARSED_ERROR;
                    }

                    xs::log::info("Content cooked");

                    break;
                }
                default:
                {
                    xs::log::error("Invalid option: {0}", argv[p]);
                    return EngineExecution::ARGUMENTS_PARSED_ERROR;
                }
                }
            }

            p++;
        }

        return EngineExecution::ARUGMENTS_PARSED;
    }
#endif

    return EngineExecution::DEFAULT;
}

int xs::main(int argc, char* argv[])
{	
    // We always like to have some debug logging available
    log::initialize();

    // When arguments have been parsed the engine can be in 2 states
    // Either the arguments have been parsed successfully and we can close the engine
    // A parsing error has occurred and the engine will close down with error code 1
    // If there are no arguments to be parsed we continue with the default behavior which is run the game
    EngineExecution execution = argument_parser(argc, argv);
    switch (execution)
    {
    case EngineExecution::ARGUMENTS_PARSED_ERROR: return 1;
    case EngineExecution::ARUGMENTS_PARSED: return 0;
    case EngineExecution::DEFAULT:
        // pass through
        break;
    }

    // Default engine initialization
    account::initialize();
    fileio::initialize();
    data::initialize();
    script::configure();
    device::initialize();
    render::initialize();
    input::initialize();
    audio::initialize();
    inspector::initialize();
    script::initialize();

    // Run
    auto prev_time = chrono::high_resolution_clock::now();
    while (!device::should_close())
	{
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = current_time - prev_time;
        prev_time = current_time;
        auto dt = std::chrono::duration<double>(elapsed).count();
        if (dt > 0.03333) dt = 0.03333;

        device::poll_events();
        input::update(dt);
        if (!inspector::paused())
        {
            render::clear();
            script::update(dt);
            audio::update(dt);
            script::render();
        }
        device::begin_frame();
        render::render();
        inspector::render(float(dt));
        device::end_frame();
	}

    // Close down the engine
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
