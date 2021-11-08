#include "device.h"
#include "device_pc.h"
#include "log.h"
#include "opengl.h"
#include "configuration.h"
#include <GLFW/glfw3.h>

namespace xs::device::internal
{
	GLFWwindow* window = nullptr;
}

using namespace xs;

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
		spdlog::critical("GLFW init failed");
		assert(false);
		exit(EXIT_FAILURE);
	}

	log::info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

	glfwSetErrorCallback(errorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// TODO: glfwWindowHint(GLFW_SAMPLES, 8);

	#if defined(DEBUG)	
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

	// TODO: Check build configuration
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// TODO: Get settings
	const auto width = configuration::width * configuration::multiplier;
	const auto height = configuration::height * configuration::multiplier;
	internal::window = glfwCreateWindow(
		width,
		height,
		xs::configuration::title.c_str(),
		nullptr,
		nullptr);

	if (!internal::window)
	{
		spdlog::critical("GLFW window could not be created");
		glfwTerminate();
		assert(false);
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(internal::window);
	glfwSwapInterval(1);

	int major = glfwGetWindowAttrib(internal::window, GLFW_CONTEXT_VERSION_MAJOR);
	int minor = glfwGetWindowAttrib(internal::window, GLFW_CONTEXT_VERSION_MINOR);
	int revision = glfwGetWindowAttrib(internal::window, GLFW_CONTEXT_REVISION);
	spdlog::info("GLFW OpenGL context version {}.{}.{}", major, minor, revision);

	// OpenGL init here
	if (!gladLoadGLLoader(GLADloadproc(glfwGetProcAddress)))
	{
		spdlog::critical("GLAD failed to initialize OpenGL context");
		assert(false);
		exit(EXIT_FAILURE);
	}

	logOpenGLVersionInfo();
	init_debug_messages();
}

void device::shutdown()
{
	glfwDestroyWindow(internal::window);
	glfwTerminate();
}

void device::swap_buffers()
{
	glfwSwapBuffers(internal::window);
}

void device::poll_events()
{
	glfwPollEvents();
}

bool device::should_close()
{
	return glfwWindowShouldClose(internal::window);
}

GLFWwindow* device::get_window()
{
	return internal::window;
}
