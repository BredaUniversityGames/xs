#if defined(VULKAN)
#include "render_internal.h"
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <stb/stb_easy_font.h>
#include <map>
#include <set>
#include "configuration.h"
#include "fileio.h"
#include "log.h"
#include "tools.h"
#include "profiler.h"
#include "render.h"
#include "device_pc.h"
#include <optional>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

using namespace glm;

namespace xs::render::internal
{
	struct vertex_format { vec3 position; vec4 color; };

	int       width                 = -1;
	int       height                = -1;
	int       triangles_count       =  0;
	int	      lines_count           =  0;
	int       triangles_begin_count =  0;
	int       lines_begin_count     =  0;
	int const lines_max             =  16000;
	int const triangles_max         =  21800;

	vertex_format			 vertex_array[lines_max * 2];
	vertex_format			 triangles_array[triangles_max * 3];
	primitive				 current_primitive = primitive::none;

	vec4					 current_color;

	uint32_t                 amount_gpu_devices    =  0;
	VkInstance               instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice         current_gpu;
	VkDevice                 current_device;
	VkSurfaceKHR             surface;
	VkQueue                  graphics_queue;
	VkQueue                  present_queue;

	std::vector<std::string>      supportedInstanceExtensions;
	std::vector<const char*>      instanceExtensions;
}

using namespace xs;
using namespace xs::render::internal;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) 
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		spdlog::info("[ValidationLayer]: {}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		spdlog::warn("[ValidationLayer]: {}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		spdlog::critical("[ValidationLayer]: {}", pCallbackData->pMessage);

		break;
	default:
		break;
	}
	return VK_FALSE;
}

VkResult create_debug_utils(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_utils(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		func(instance, debugMessenger, pAllocator);
	}
}

int rate_suitable_gpu(VkPhysicalDevice device) 
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
	{
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) 
		return 0;

	return score;
}

void create_instance()
{
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = configuration::title().c_str();
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "xs";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	// Get extensions supported by the instance and store for later use
	uint32_t ext_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
	if (ext_count > 0)
	{
		std::vector<VkExtensionProperties> extensions(ext_count);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, &extensions.front()) == VK_SUCCESS)
		{
			for (const VkExtensionProperties& extension : extensions)
			{
				supportedInstanceExtensions.push_back(extension.extensionName);
			}
		}
	}

	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	for (uint32_t i = 0; i < glfw_extension_count; i++)
		xs::render::internal::instanceExtensions.push_back(glfw_extensions[i]);

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledExtensionCount = glfw_extension_count;
	instance_create_info.ppEnabledExtensionNames = glfw_extensions;
	if (instanceExtensions.size() > 0)
	{
#if defined(DEBUG)
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);	// SRS - Dependency when VK_EXT_DEBUG_MARKER is enabled
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		instance_create_info.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instance_create_info.ppEnabledExtensionNames = instanceExtensions.data();
	}

	if (vkCreateInstance(&instance_create_info, nullptr, &xs::render::internal::instance) != VK_SUCCESS)
	{
		spdlog::critical("[ValidationLayer]: Failed to create instance!");
		assert(false);
	}

#if defined(DEBUG)
	const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
	// Check if this layer is available at instance level
	uint32_t instanceLayerCount;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
	std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
	bool validationLayerPresent = false;
	for (const VkLayerProperties& layer : instanceLayerProperties) {
		if (strcmp(layer.layerName, validationLayerName) == 0) {
			validationLayerPresent = true;
			break;
		}
	}
	if (validationLayerPresent) {
		instance_create_info.ppEnabledLayerNames = &validationLayerName;
		instance_create_info.enabledLayerCount = 1;
	}
	else
	{
		spdlog::critical("[ValidationLayer]: VK_LAYER_KHRONOS_validation not present, validation is disabled");
		assert(false);
	}
#endif
}

void create_debug_handler()
{
	VkDebugUtilsMessengerCreateInfoEXT create_debug_info{};
	create_debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_debug_info.pfnUserCallback = debug_callback;
	create_debug_info.pUserData = nullptr;

	if (create_debug_utils(instance, &create_debug_info, nullptr, &debug_messenger) != VK_SUCCESS)
	{
		spdlog::critical("[ValidationLayer]: Failed to set up debug messenger!");
		assert(false);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value();
	}
};

QueueFamilyIndices pick_queue_families(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	VkBool32 presentSupport = false;
	for (const auto& queue_family : queue_families)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			indices.graphics_family = i;
		
		if (presentSupport)
			indices.present_family = i;

		if (indices.is_complete())
			break;

		i++;
	}

	return indices;
}

void create_logical_device()
{
	float queue_priority = 1.0f;

	QueueFamilyIndices indices = pick_queue_families(current_gpu);

	VkDeviceQueueCreateInfo queue_create_info{};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = indices.graphics_family.value();
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;

	char* extensions[] = {
		 VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &queue_create_info;
	device_info.queueCreateInfoCount = 1;
	device_info.ppEnabledExtensionNames = extensions;
#if defined(DEBUG)
	device_info.enabledLayerCount = static_cast<uint32_t>(instanceExtensions.size());
	device_info.ppEnabledLayerNames = instanceExtensions.data();
#else
	device_info.enabledLayerCount = 0;
#endif

	if (vkCreateDevice(current_gpu, &device_info, 0, &current_device) != VK_SUCCESS) 
	{
		spdlog::critical("[GPU]: Failed to create logical device!");
		assert(false);
	}

	vkGetDeviceQueue(current_device, indices.graphics_family.value(), 0, &graphics_queue);
}

void pick_gpu_device()
{
	vkEnumeratePhysicalDevices(instance, &amount_gpu_devices, nullptr);
	std::vector<VkPhysicalDevice> devices(amount_gpu_devices);
	vkEnumeratePhysicalDevices(instance, &amount_gpu_devices, devices.data());

	if (amount_gpu_devices == 0)
	{
		spdlog::critical("[GPU]: Failed to find GPUs with Vulkan support! ");
		assert(false);
	}


	std::multimap<int, VkPhysicalDevice> candidates;
	for (const auto& device : devices) 
	{
		int score = rate_suitable_gpu(device);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0) 
	{
		current_gpu = candidates.rbegin()->second;
	}
	else 
	{
		spdlog::critical("[GPU]: Failed to find a suitable GPU! ");
		assert(false);
	}
}

void create_surface()
{
	if (glfwCreateWindowSurface(instance, device::get_window(), nullptr, &surface) != VK_SUCCESS)
	{
		spdlog::critical("[Window]: Failed to create window surface!");
		assert(false);
	}
}

void create_present_queue()
{
	QueueFamilyIndices indices = pick_queue_families(current_gpu);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphics_family.value(), indices.present_family.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) 
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vkGetDeviceQueue(current_device, indices.present_family.value(), 0, &present_queue);
}

void xs::render::initialize()
{
	width = configuration::width();
	height = configuration::height();

	create_instance();

#if defined(DEBUG)
	create_debug_handler();
#endif

	create_surface();
	pick_gpu_device();
	create_logical_device();
	create_present_queue();
}

void xs::render::render()
{
	XS_PROFILE_SECTION("xs::render::render");
	// TODO: make a camera class
	auto w = width * 0.5f;
	auto h = height * 0.5f;
	mat4 p = ortho(-w, w, -h, h, -100.0f, 100.0f);
	mat4 v = lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 vp = p * v;

	// TODO: make it only sort when you add stuff to the queu
	std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
		[](const sprite_queue_entry& lhs, const sprite_queue_entry& rhs) {
			return lhs.z < rhs.z;
		});

	int count = 0;
	for (auto i = 0; i < sprite_queue.size(); i++)
	{
		const auto& spe = sprite_queue[i];
		const auto& sprite = sprites[spe.sprite_id];
		const auto& image = images[sprite.image_id];

		auto from_x = 0.0;
		auto from_y = 0.0;
		auto to_x = image.width * (sprite.to.x - sprite.from.x) * spe.scale;
		auto to_y = image.height * (sprite.to.y - sprite.from.y) * spe.scale;

		auto from_u = sprite.from.x;
		auto from_v = sprite.from.y;
		auto to_u = sprite.to.x;
		auto to_v = sprite.to.y;

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_x))
			std::swap(from_u, to_u);

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_y))
			std::swap(from_v, to_v);

		vec4 add_color = to_vec4(spe.add_color);
		vec4 mul_color = to_vec4(spe.mul_color);
	}
}

void xs::render::shutdown()
{
	vkDestroyDevice(current_device, nullptr);

#if defined(DEBUG)
	destroy_debug_utils(instance, debug_messenger, nullptr);
#endif

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	// TODO: Delete all images
}

void xs::render::clear()
{
	lines_count = 0;
	triangles_count = 0;
	sprite_queue.clear();
}

void xs::render::internal::create_texture_with_data(xs::render::internal::image& img, uchar* data)
{

}

void xs::render::begin(primitive p)
{
	if (current_primitive == primitive::none)
	{
		current_primitive = p;
		triangles_begin_count = 0;
		lines_begin_count = 0;
	}
	else
	{
		log::error("Renderer begin()/end() mismatch! Primitive already active in begin().");
	}
}

void xs::render::vertex(double x, double y)
{
	if (current_primitive == primitive::triangles && triangles_count < triangles_max - 1)
	{
		const uint idx = triangles_count * 3;
		triangles_array[idx + triangles_begin_count].position = { x, y, 0.0f };
		triangles_array[idx + triangles_begin_count].color = current_color;
		triangles_begin_count++;
		if (triangles_begin_count == 3)
		{
			triangles_begin_count = 0;
			triangles_count++;
		}
	}
	else if (current_primitive == primitive::lines && lines_count < lines_max)
	{
		if (lines_begin_count == 0)
		{
			vertex_array[lines_count * 2].position = { x, y, 0.0f };
			vertex_array[lines_count * 2].color = current_color;
			lines_begin_count++;
		}
		else if (lines_begin_count == 1)
		{
			vertex_array[lines_count * 2 + 1].position = { x, y, 0.0f };
			vertex_array[lines_count * 2 + 1].color = current_color;
			lines_begin_count++;
			lines_count++;
		}
		else
		{
			// assert(lines_begin_count > 1 && lines_count > 1);
			vertex_array[lines_count * 2].position = vertex_array[lines_count * 2 - 1].position;
			vertex_array[lines_count * 2].color = vertex_array[lines_count * 2 - 1].color;
			vertex_array[lines_count * 2 + 1].position = { x, y, 0.0f };
			vertex_array[lines_count * 2 + 1].color = current_color;
			lines_begin_count++;
			lines_count++;
		}
	}
}

void xs::render::end()
{
	if (current_primitive == primitive::none)
	{
		log::error("Renderer begin()/end() mismatch! No primitive active in end().");
		return;
	}

	current_primitive = primitive::none;
	if (triangles_begin_count != 0 /* TODO: lines */)
	{
		log::error("Renderer vertex()/end() mismatch!");
	}
}

void xs::render::set_color(double r, double g, double b, double a)
{
	current_color.r = static_cast<float>(r);
	current_color.g = static_cast<float>(g);
	current_color.b = static_cast<float>(b);
	current_color.a = static_cast<float>(a);
}

void xs::render::set_color(color c)
{
	static float inverseColorValue = 1.f / 255.0f;
	current_color.r = c.r * inverseColorValue;
	current_color.g = c.g * inverseColorValue;
	current_color.b = c.b * inverseColorValue;
	current_color.a = c.a * inverseColorValue;
}

void xs::render::line(double x0, double y0, double x1, double y1)
{
	if (lines_count < lines_max)
	{
		vertex_array[lines_count * 2].position = { x0, y0, 0.0f };
		vertex_array[lines_count * 2 + 1].position = { x1, y1, 0.0f };
		vertex_array[lines_count * 2].color = current_color;
		vertex_array[lines_count * 2 + 1].color = current_color;
		++lines_count;
	}
}

void xs::render::text(const std::string& text, double x, double y, double size)
{
	struct stbVec
	{
		float x;
		float y;
		float z;
		unsigned char color[4];
	};

	static stbVec vertexBuffer[2048];

	const auto n = text.length();
	char* asChar = new char[n + 1];
	strcpy(asChar, text.c_str());
	const int numQuads = stb_easy_font_print(0, 0, asChar, nullptr, vertexBuffer, sizeof(vertexBuffer));
	delete[] asChar;

	const vec3 origin(x, y, 0.0f);
	const auto s = static_cast<float>(size);
	for (int i = 0; i < numQuads; i++)
	{
		const auto& v0 = vertexBuffer[i * 4 + 0];
		const auto& v1 = vertexBuffer[i * 4 + 1];
		const auto& v2 = vertexBuffer[i * 4 + 2];
		const auto& v3 = vertexBuffer[i * 4 + 3];

		const uint idx = triangles_count * 3;
		triangles_array[idx + 0].position = s * vec3(v0.x, -v0.y, v0.z) + origin;
		triangles_array[idx + 2].position = s * vec3(v1.x, -v1.y, v1.z) + origin;
		triangles_array[idx + 1].position = s * vec3(v2.x, -v2.y, v2.z) + origin;
		triangles_array[idx + 3].position = s * vec3(v2.x, -v2.y, v2.z) + origin;
		triangles_array[idx + 4].position = s * vec3(v3.x, -v3.y, v3.z) + origin;
		triangles_array[idx + 5].position = s * vec3(v0.x, -v0.y, v0.z) + origin;

		triangles_array[idx + 0].color = current_color;
		triangles_array[idx + 1].color = current_color;
		triangles_array[idx + 2].color = current_color;

		triangles_array[idx + 3].color = current_color;
		triangles_array[idx + 4].color = current_color;
		triangles_array[idx + 5].color = current_color;

		triangles_count += 2;

		if (triangles_count >= triangles_max)
			return;
	}
}
#endif
