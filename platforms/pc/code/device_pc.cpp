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


/*
namespace xs::device::internal
{
	GLFWwindow* window = nullptr;
	int	width;
	int	height;

}

using namespace xs;
using namespace xs::device;


static void errorCallback(int error, const char* description)
{
	fputs(description, stderr);
}

void logOpenGLVersionInfo()
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
	if (!glfwInit())
	{
		log::critical("GLFW init failed");
		assert(false);
		exit(EXIT_FAILURE);
	}

	log::info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

	glfwSetErrorCallback(errorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	if(configuration::on_top())
		glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

#if defined(DEBUG)	
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	internal::width = configuration::width() * configuration::multiplier();
	internal::height = configuration::height() * configuration::multiplier();
	internal::window = glfwCreateWindow(
		internal::width,
		internal::height,
		configuration::title().c_str(),
		nullptr,
		nullptr);

	if (!internal::window)
	{
		log::critical("GLFW window could not be created");
		glfwTerminate();
		assert(false);
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(internal::window);
	//glfwSwapInterval(1); <- vsync by default, but more reliable when not explicitly set?

	int major = glfwGetWindowAttrib(internal::window, GLFW_CONTEXT_VERSION_MAJOR);
	int minor = glfwGetWindowAttrib(internal::window, GLFW_CONTEXT_VERSION_MINOR);
	int revision = glfwGetWindowAttrib(internal::window, GLFW_CONTEXT_REVISION);
	log::info("GLFW OpenGL context version {}.{}.{}", major, minor, revision);

#if !defined(__APPLE__)
	// OpenGL init here
	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)))
	{
		log::critical("GLAD failed to initialize OpenGL context");
		assert(false);
		exit(EXIT_FAILURE);
	}
#endif

	logOpenGLVersionInfo();
	init_debug_messages();


	//Set application icon
	GLFWimage image[2];
	std::string path = fileio::get_path("[shared]/images/icon.png");
	image[0].pixels = stbi_load(path.c_str(), &image[0].width, &image[0].height, 0, 4);
	path = fileio::get_path("[shared]/images/icon_tiny.png");
	image[1].pixels = stbi_load(path.c_str(), &image[1].width, &image[1].height, 0, 4);
	glfwSetWindowIcon(internal::window, 2, image);
	stbi_image_free(image[0].pixels);
	stbi_image_free(image[1].pixels);
}

void device::shutdown()
{
	glfwDestroyWindow(internal::window);
	glfwTerminate();
}

void device::begin_frame() {}

void device::end_frame()
{
	XS_PROFILE_FUNCTION();
	glfwSwapBuffers(internal::window);
}

void device::poll_events()
{
	glfwPollEvents();
}

bool device::can_close()
{
	return true;
}

bool device::request_close()
{
	glfwSetWindowShouldClose(internal::window, GL_TRUE);
	return true;
}

bool device::should_close()
{
	return glfwWindowShouldClose(internal::window);
}

GLFWwindow* device::get_window()
{
	return internal::window;
}

int xs::device::get_width()
{
	return internal::width;
}

int xs::device::get_height()
{
	return internal::height;
}

*/

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
		case SDL_EVENT_QUIT:
			quit = true;
			break;
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

