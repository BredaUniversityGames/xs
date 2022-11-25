#include "device.h"
#include "device_pc.h"
#include "log.h"
#include "configuration.h"
#include "fileio.h"
#include "data.h"
#include "profiler.h"
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

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

	if (configuration::on_top())
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
	spdlog::info("GLFW Vulkan context version {}.{}.{}", major, minor, revision);

	//Set application icon
	GLFWimage image[2];
	std::string path = fileio::get_path("[games]/shared/images/icon.png");
	image[0].pixels = stbi_load(path.c_str(), &image[0].width, &image[0].height, 0, 4);
	path = fileio::get_path("[games]/shared/images/icon_tiny.png");
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

void device::swap_buffers()
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
	glfwSetWindowShouldClose(internal::window, true);
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
