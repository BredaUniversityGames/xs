#include "device.hpp"
#include "device_pc.hpp"
#include "log.hpp"
#include "opengl.hpp"
#include "configuration.hpp"
#include "fileio.hpp"
#include "data.hpp"
#include "profiler.hpp"
#include <imgui.h>
#include "imgui_impl.h"
#include <stb/stb_image.h>
#include <SDL3/SDL.h>

namespace xs::device::internal
{
	SDL_Window* window = nullptr;
	SDL_GLContext context = nullptr;
	int	width = -1;
	int	height = -1;
	bool quit = false;
}

using namespace xs;
using namespace xs::device;

void log_opengl_version_info()
{
	const auto vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const auto renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	const auto shaderVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

	log::info("OpenGL Vendor {}", vendor);
	log::info("OpenGL Renderer {}", renderer);
	log::info("OpenGL Version {}", version);
	log::info("OpenGL Shader Version {}", shaderVersion);
}



void device::initialize()
{
	// Switch to SDL
	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		log::critical("SDL init failed");
		assert(false);
		exit(EXIT_FAILURE);
	}

	log::info("SDL version {}.{}.{}", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

	// Set OpenGL version
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Set window size
	internal::width = configuration::width() * configuration::multiplier();
	internal::height = configuration::height() * configuration::multiplier();

	// Create window
	internal::window = SDL_CreateWindow(
		configuration::title().c_str(),
		internal::width,
		internal::height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	
	if (!internal::window)
	{
		log::critical("SDL window could not be created");
		SDL_Quit();
		assert(false);
		exit(EXIT_FAILURE);
	}

	// Set OpenGL context
	internal::context = SDL_GL_CreateContext(internal::window);
	SDL_GL_MakeCurrent(internal::window, internal::context);
	SDL_GL_SetSwapInterval(1);

	// OpenGL init here	
	if (!gladLoadGL())
	{
		log::critical("GLAD failed to initialize OpenGL context");
		assert(false);
		exit(EXIT_FAILURE);
	}

	log_opengl_version_info();
	init_debug_messages();

	SDL_ShowWindow(internal::window);

	// Set application icon
	std::string path = fileio::get_path("[shared]/images/icon.png");
	SDL_Surface* icon = SDL_LoadBMP(path.c_str());
	SDL_SetWindowIcon(internal::window, icon);
	SDL_DestroySurface(icon);
}

void device::shutdown()
{
	SDL_DestroyWindow(internal::window);
	SDL_Quit();
}

void device::begin_frame() {}

void device::end_frame()
{
	XS_PROFILE_FUNCTION();
	SDL_GL_SwapWindow(internal::window);	
}

void device::poll_events()
{
	SDL_Event event;
	bool quit = false;
	while (SDL_PollEvent(&event) && !quit)
	{
		switch (event.type)
		{
			// Quit event
		case SDL_EVENT_QUIT:
			quit = true;
			break;
			// Window resize event
		case SDL_EVENT_WINDOW_RESIZED:
			if (event.window.windowID == SDL_GetWindowID(internal::window))
			{
				internal::width = event.window.data1;
				internal::height = event.window.data2;
				SDL_SetWindowSize(internal::window, internal::width, internal::height);
				SDL_GL_MakeCurrent(internal::window, internal::context);
			}
			break;
			// Window focus event
		}


		ImGui_Impl_ProcessEvent(&event);
	}

	internal::quit = quit;
}

bool device::can_close()
{
	return true;
}

bool device::request_close()
{
	SDL_Event event;
	event.type = SDL_EVENT_QUIT;
	SDL_PushEvent(&event);
	return true;
}

bool device::should_close()
{
	return internal::quit;
}

SDL_Window* device::get_window()
{
	return internal::window;
}

SDL_GLContextState* xs::device::get_context()
{
	return internal::context;	
}

int xs::device::get_width()
{
	return internal::width;
}

int xs::device::get_height()
{
	return internal::height;
}

double device::hdpi_scaling()
{
	return SDL_GetWindowDisplayScale(internal::window);
}

void device::set_fullscreen(bool fullscreen)
{
	if (!internal::window)
		return;

	if (fullscreen)
	{
		// Use SDL_WINDOW_FULLSCREEN for "fullscreen desktop" mode
		// This maintains the desktop resolution and lets the OS handle it
		SDL_SetWindowFullscreen(internal::window, true);
		
		// Get the actual window size in fullscreen mode
		SDL_GetWindowSize(internal::window, &internal::width, &internal::height);
	}
	else
	{
		// Restore windowed mode
		SDL_SetWindowFullscreen(internal::window, false);
		
		// Restore windowed size
		internal::width = configuration::width() * configuration::multiplier();
		internal::height = configuration::height() * configuration::multiplier();
		SDL_SetWindowSize(internal::window, internal::width, internal::height);
	}
}

bool device::toggle_on_top()
{
	if (!internal::window)
		return false;

	auto flags = SDL_GetWindowFlags(internal::window);
	bool is_on_top = (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
	is_on_top = !is_on_top;
	SDL_SetWindowAlwaysOnTop(internal::window, is_on_top);
	return is_on_top;
}

