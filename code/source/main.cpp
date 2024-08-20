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
#include "editor.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

using namespace std;

namespace xs
{
	void initialize();
	void shutdown();
	void run();
    int main(int argc, char* argv[]);
}

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

void xs::initialize()
{
	log::initialize();
	editor::initialize();
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
}

void xs::shutdown()
{
    inspector::shutdown();
    audio::shutdown();
    input::shutdown();
    render::shutdown();
    device::shutdown();
    script::shutdown();
    account::shutdown();
    data::shutdown();
}

void xs::run()
{
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

        if (inspector::should_restart())
        {
            xs::shutdown();
            xs::initialize();
        }
    }
}

int xs::main(int argc, char* argv[])
{	
	xs::initialize();
    xs::run();
	xs::shutdown();
	return 0;
}
