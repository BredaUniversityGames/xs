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



//////////////////////////////////////////////////////////////////////////
//// C++ game hack
//////////////////////////////////////////////////////////////////////////
class Game
{
public:
    Game() {
        mesh = xs::render::load_mesh("[games]/samples/xs3d/assets/vespa/vespa.obj");
        image = xs::render::load_image("[games]/samples/xs3d/assets/vespa/vespa.png");
    }
    ~Game() {}

    void update(double dt)
	{
        xs::render::color white;
        white.integer_value = 0xFFFFFFFF;
        xs::render::color black;
        black.integer_value = 0x000000FF;
        xs::render::render_directional_light(
            glm::value_ptr(glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f))),
            glm::value_ptr(glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)))
        );

        xs::render::render_mesh(
            mesh,
            image,
            glm::value_ptr(glm::mat4(1.0f)),
            white, black);

        static float angle = 0.0f;
        angle += 1.0f * (float)dt;
        float r = 15.0f;
        glm::vec3 eye(cos(angle) * r, sin(angle) * r, 10);
        glm::mat4 view = glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
        xs::render::set_3d_view(glm::value_ptr(view));
        xs::render::set_3d_projection(glm::value_ptr(projection));
		// Update game
	}

    void render()
	{
		// Render game
	}

    int mesh;
    int image;
};



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

    // Game
    Game game;


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
            game.update(dt);
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
