#include "device.hpp"
#include "device_sdl.hpp"
#include "log.hpp"
#include "opengl/opengl.hpp"
#include "configuration.hpp"
#include "fileio.hpp"
#include "data.hpp"
#include "profiler.hpp"
#include "inspector.hpp"
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

	// Where the window is and how big when it is not maximized or fullscreen, to come back to next time
	SDL_Rect restored = {};
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



#if defined(EDITOR)
namespace
{
	// Whether a rectangle is all on one screen, in the part windows can use (not under the taskbar)
	bool fits_on_a_screen(const SDL_Rect& rect)
	{
		const SDL_DisplayID display = SDL_GetDisplayForRect(&rect);
		SDL_Rect usable = {};
		if (display == 0 || !SDL_GetDisplayUsableBounds(display, &usable))
			return false;
		return rect.x >= usable.x && rect.y >= usable.y &&
		       rect.x + rect.w <= usable.x + usable.w && rect.y + rect.h <= usable.y + usable.h;
	}

	// Keeps the window's place and size, unless it is maximized, fullscreen or minimized: then it keeps the ones it
	// goes back to
	void remember_window_placement()
	{
		const auto flags = SDL_GetWindowFlags(internal::window);
		if (flags & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED))
			return;
		SDL_GetWindowPosition(internal::window, &internal::restored.x, &internal::restored.y);
		SDL_GetWindowSize(internal::window, &internal::restored.w, &internal::restored.h);
	}
}
#endif

void device::initialize()
{
	// App ID must match the .desktop file name (xs.desktop) so Wayland compositors
	// like GNOME can find the icon; they ignore SDL_SetWindowIcon
	SDL_SetHint(SDL_HINT_APP_ID, "xs");

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
#if defined(EDITOR)
	// In editor builds, restore saved window size from user settings
	int saved_width = (int)data::get_number("Editor.WindowWidth", data::type::user);
	int saved_height = (int)data::get_number("Editor.WindowHeight", data::type::user);

	if (saved_width > 0 && saved_height > 0)
	{
		internal::width = saved_width;
		internal::height = saved_height;
	}
	else
	{
		internal::width = configuration::width() * configuration::multiplier();
		internal::height = configuration::height() * configuration::multiplier();
	}
#else
	internal::width = configuration::width() * configuration::multiplier();
	internal::height = configuration::height() * configuration::multiplier();
#endif

	// Create window, hidden until it is in its place
	internal::window = SDL_CreateWindow(
		configuration::title().c_str(),
		internal::width,
		internal::height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
	
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

#if defined(EDITOR)
	// Back where it was last time, if that is still all on one screen (one may have been unplugged since)
	if (data::get_bool("Editor.WindowPlaced", data::type::user))
	{
		const SDL_Rect saved = {
			(int)data::get_number("Editor.WindowX", data::type::user),
			(int)data::get_number("Editor.WindowY", data::type::user),
			internal::width,
			internal::height };
		if (fits_on_a_screen(saved))
			SDL_SetWindowPosition(internal::window, saved.x, saved.y);
	}
#endif

	SDL_ShowWindow(internal::window);

#if defined(EDITOR)
	remember_window_placement();
	if (data::get_bool("Editor.WindowMaximized", data::type::user))
		SDL_MaximizeWindow(internal::window);
#endif

	// Set application icon (load PNG via stb_image, then create SDL surface)
	{
		std::string path = fileio::get_path("[shared]/images/icon.png");
		int w, h, channels;
		unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
		if (pixels)
		{
			SDL_Surface* icon = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
			if (icon)
			{
				SDL_SetWindowIcon(internal::window, icon);
				SDL_DestroySurface(icon);
			}
			stbi_image_free(pixels);
		}
	}
}

void device::shutdown()
{
#if defined(EDITOR)
	// Save where the window is to user settings before shutdown in editor builds: its place and size when not
	// maximized, and whether it is
	if (internal::restored.w > 0 && internal::restored.h > 0)
	{
		const bool maximized = (SDL_GetWindowFlags(internal::window) & SDL_WINDOW_MAXIMIZED) != 0;
		data::set_number("Editor.WindowWidth", internal::restored.w, data::type::user);
		data::set_number("Editor.WindowHeight", internal::restored.h, data::type::user);
		data::set_number("Editor.WindowX", internal::restored.x, data::type::user);
		data::set_number("Editor.WindowY", internal::restored.y, data::type::user);
		data::set_bool("Editor.WindowPlaced", true, data::type::user);
		data::set_bool("Editor.WindowMaximized", maximized, data::type::user);
	}
	data::save_of_type(data::type::user);
#endif

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
				// Update OpenGL viewport to match new window size
				glViewport(0, 0, internal::width, internal::height);
			}
#if defined(EDITOR)
			remember_window_placement();
#endif
			break;
#if defined(EDITOR)
		case SDL_EVENT_WINDOW_MOVED:
			remember_window_placement();
			break;
#endif
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

void xs::device::set_window_size(int w, int h)
{
	if (!internal::window)
		return;
	
	internal::width = w;
	internal::height = h;
	SDL_SetWindowSize(internal::window, w, h);
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

