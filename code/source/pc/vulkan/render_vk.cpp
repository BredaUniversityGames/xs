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


#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <imgui_impl_glfw.h>
#include "vulkan/render_vk.h"
#include "vulkan/texture_vk.h"
#include "device.h"
#include <imgui_impl.h>
#include <vulkan/buffer_vk.h>

using namespace glm;
namespace xs::render::internal
{
	struct vertex_format 
	{ 
		vec3 position; 
		vec4 color;

		static VkVertexInputBindingDescription get_binding_description() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(vertex_format);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(vertex_format, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(vertex_format, color);

			return attributeDescriptions;
		}
	};

	struct sprite_vtx_format
	{
		vec3 position;
		vec2 texture;
		vec4 add_color;
		vec4 mul_color;

		static VkVertexInputBindingDescription get_binding_description() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(sprite_vtx_format);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 4> get_attribute_descriptions() {
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(sprite_vtx_format, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(sprite_vtx_format, texture);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(sprite_vtx_format, mul_color);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(sprite_vtx_format, add_color);

			return attributeDescriptions;
		}
	};
	unsigned int			sprite_program = 0;
	int const				sprite_trigs_max = 21800;
	int						sprite_trigs_count = 0;
	sprite_vtx_format		sprite_trigs_array[sprite_trigs_max * 3];

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

	uint32_t                 current_frame         =  0;
	uint32_t                 amount_gpu_devices    =  0;
	VkInstance               instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice         current_gpu;
	VkDevice                 current_device;
	VkSurfaceKHR             surface;
	VkQueue                  graphics_queue;
	VkQueue                  present_queue;
	VkSwapchainKHR           swapchain;
	bool				     swapchain_rebuild;
	VkFormat                 swapchain_image_format;
	VkExtent2D               swapchain_extent;
	VkPipelineLayout         triangle_pipeline_layout;
	VkPipelineLayout         sprite_pipeline_layout;
	VkRenderPass             render_pass;
	VkPipeline               triangle_graphics_pipeline;
	VkPipeline               sprite_graphics_pipeline;
	VkCommandPool            command_pool;
	uint32_t				 image_index;
	VkDescriptorPool         descriptor_pool;
	VkDescriptorSetLayout	 ubo_descriptor_set_layout;
	VkDescriptorSetLayout	 texture_descriptor_set_layout;
	buffer					 vertex_buffer_triangles;
	buffer					 vertex_buffer_lines;

	struct uniform_vertex { alignas(16) mat4 view_projection_matrix; }; 
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		bool is_complete() {
			return graphics_family.has_value() && present_family.has_value();
		}
	};
	
	std::vector<VkDescriptorSet>   descriptor_sets;
	std::vector<buffer>            uniform_buffers;
	std::vector<void*>             uniform_buffers_mapped;
	std::vector<VkCommandBuffer>   command_buffers;
	std::vector<VkImage>           swapchain_images;
	std::vector<VkImageView>       swapchain_image_views;
	std::vector<VkFramebuffer>     swapchain_framebuffers;
	std::vector<VkSemaphore>       image_available_semaphores;
	std::vector<VkSemaphore>       render_finished_semaphores;
	std::vector<VkFence>           in_flight_fences;
	std::vector<std::string>       supported_instance_extensions;
	std::vector<const char*>       instance_extensions;
	std::unordered_map<uint32_t, texture>           textures;
	const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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

void create_debug_handler(VkDebugUtilsMessengerCreateInfoEXT& create_debug_info)
{
	create_debug_info = {};
	create_debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_debug_info.pfnUserCallback = debug_callback;
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
				supported_instance_extensions.push_back(extension.extensionName);
			}
		}
	}

	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	for (uint32_t i = 0; i < glfw_extension_count; i++)
		xs::render::internal::instance_extensions.push_back(glfw_extensions[i]);

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledExtensionCount = glfw_extension_count;
	instance_create_info.ppEnabledExtensionNames = glfw_extensions;
	if (instance_extensions.size() > 0)
	{
#if defined(DEBUG)
		instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);	// SRS - Dependency when VK_EXT_DEBUG_MARKER is enabled
		instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		instance_create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
		instance_create_info.ppEnabledExtensionNames = instance_extensions.data();
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
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
	if (validationLayerPresent) {
		instance_create_info.ppEnabledLayerNames = &validationLayerName;
		instance_create_info.enabledLayerCount = 1;
		create_debug_handler(debug_create_info);
		instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
	}
	else
	{
		spdlog::critical("[ValidationLayer]: VK_LAYER_KHRONOS_validation not present, validation is disabled");
		assert(false);
	}
#endif

	if (vkCreateInstance(&instance_create_info, nullptr, &xs::render::internal::instance) != VK_SUCCESS)
	{
		spdlog::critical("[ValidationLayer]: Failed to create instance!");
		assert(false);
	}
}

SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device) 
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

bool check_device_extension_support(VkPhysicalDevice device) 
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());

	for (const auto& extension : availableExtensions) 
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) 
{
	for (const auto& available_format : available_formats)
	{
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_format;
	}

	return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_presentModes) 
{
	for (const auto& available_presentMode : available_presentModes)
	{
		if (available_presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return available_presentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) 
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(device::get_window(), &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

QueueFamilyIndices pick_queue_families(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	VkBool32 present_support = false;
	for (const auto& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
			indices.graphics_family = i;
		
		bool extensions_supported = check_device_extension_support(device);
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

		if (present_support)
			indices.present_family = i;

		bool swapchain_adequate = false;
		if (extensions_supported)
		{
			SwapChainSupportDetails swapChainSupport = query_swapchain_support(device);
			swapchain_adequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
		}

		if (indices.is_complete() && extensions_supported && swapchain_adequate)
			break;

		i++;
	}

	return indices;
}

VkShaderModule create_shader_module(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	if (vkCreateShaderModule(current_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		spdlog::critical("[Shader]: Failed to create shader module!");
		assert(false);
	}

	return shader_module;
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

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &queue_create_info;
	device_info.queueCreateInfoCount = 1;
	device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	device_info.ppEnabledExtensionNames = device_extensions.data();
#if defined(DEBUG)
	device_info.enabledLayerCount = static_cast<uint32_t>(instance_extensions.size());
	device_info.ppEnabledLayerNames = instance_extensions.data();
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

void create_swapchain()
{
	SwapChainSupportDetails swapchain_support = query_swapchain_support(current_gpu);

	VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes);
	VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities);

	uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

	if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
	{
		image_count = swapchain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = pick_queue_families(current_gpu);
	uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

	if (indices.graphics_family != indices.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0; // Optional
		create_info.pQueueFamilyIndices = nullptr; // Optional
	}
	create_info.preTransform = swapchain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(current_device, &create_info, nullptr, &swapchain) != VK_SUCCESS)
	{
		spdlog::critical("[Swapchain]: Failed to create swapchain");
		assert(false);
	}

	vkGetSwapchainImagesKHR(current_device, swapchain, &image_count, nullptr);
	swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(current_device, swapchain, &image_count, swapchain_images.data());

	swapchain_image_format = surface_format.format;
	swapchain_extent = extent;
}

uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(current_gpu, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void create_vertex_buffer() 
{
	vertex_buffer_triangles.initialize(sizeof(triangles_array[0]) * (static_cast<unsigned long long>(triangles_max) * 3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vertex_buffer_lines.initialize(sizeof(vertex_array[0]) * (static_cast<unsigned long long>(lines_max) * 2), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void update_vertex_buffer()
{
	vertex_buffer_triangles.upload_data(&triangles_array[0]);
	vertex_buffer_lines.upload_data(&vertex_array[0]);
}

void create_sprite_graphics_pipeline()
{
	auto vs_str = xs::fileio::read_binary_file("[games]/shared/shaders/v_sprite.spv");
	auto fs_str = xs::fileio::read_binary_file("[games]/shared/shaders/f_sprite.spv");

	VkShaderModule vert_shader_module = create_shader_module(vs_str);
	VkShaderModule frag_shader_module = create_shader_module(fs_str);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	auto bindingDescription = sprite_vtx_format::get_binding_description();
	auto attributeDescriptions = sprite_vtx_format::get_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
	vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamic_states =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	VkDescriptorSetLayout set_layouts[] = { ubo_descriptor_set_layout, texture_descriptor_set_layout };

	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = set_layouts;
	pipeline_layout_info.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(current_device, &pipeline_layout_info, nullptr, &sprite_pipeline_layout) != VK_SUCCESS)
	{
		spdlog::critical("[Pipeline]: Failed to create pipeline layout!");
		assert(false);
	}

	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = sprite_pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(current_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &sprite_graphics_pipeline) != VK_SUCCESS)
	{
		spdlog::critical("[Pipeline]: Failed to create graphics pipeline!");
		assert(false);
	}

	vkDestroyShaderModule(current_device, frag_shader_module, nullptr);
	vkDestroyShaderModule(current_device, vert_shader_module, nullptr);
}

void create_graphics_pipeline() 
{
	auto vs_str = xs::fileio::read_binary_file("[games]/shared/shaders/v_triangle.spv");
	auto fs_str = xs::fileio::read_binary_file("[games]/shared/shaders/f_triangle.spv");

	VkShaderModule vert_shader_module = create_shader_module(vs_str);
	VkShaderModule frag_shader_module = create_shader_module(fs_str);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	auto bindingDescription = vertex_format::get_binding_description();
	auto attributeDescriptions = vertex_format::get_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertex_input_info.pVertexBindingDescriptions = &bindingDescription;
	vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamic_states =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &ubo_descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(current_device, &pipeline_layout_info, nullptr, &triangle_pipeline_layout) != VK_SUCCESS)
	{
		spdlog::critical("[Pipeline]: Failed to create pipeline layout!");
		assert(false);
	}

	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.layout = triangle_pipeline_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(current_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &triangle_graphics_pipeline) != VK_SUCCESS)
	{
		spdlog::critical("[Pipeline]: Failed to create graphics pipeline!");
		assert(false);
	}

	vkDestroyShaderModule(current_device, frag_shader_module, nullptr);
	vkDestroyShaderModule(current_device, vert_shader_module, nullptr);
}

void create_render_pass() 
{
	VkAttachmentDescription color_attachment{};
	color_attachment.format = swapchain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	if (vkCreateRenderPass(current_device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) 
	{
		spdlog::critical("[Pipeline]: Failed to create render pass!");
		assert(false);
	}
}

void create_image_views() 
{
	swapchain_image_views.resize(swapchain_images.size());
	int i = 0;
	for (const auto& swapchain_image : swapchain_images)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swapchain_image;
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swapchain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(current_device, &create_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS)
		{
			spdlog::critical("[Swapchain]: Failed to create image views! ");
			assert(false);
		}

		i++;
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

void create_frame_buffers()
{
	swapchain_framebuffers.resize(swapchain_image_views.size());

	for (size_t i = 0; i < swapchain_image_views.size(); i++) 
	{
		VkImageView attachments[] = 
		{
			swapchain_image_views[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = render_pass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchain_extent.width;
		framebufferInfo.height = swapchain_extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(current_device, &framebufferInfo, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) 
		{
			spdlog::critical("[Swapchain]: Failed to create framebuffer!");
			assert(false);
		}
	}
}

void create_command_pool() 
{
	QueueFamilyIndices queue_family_indices = pick_queue_families(current_gpu);

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

	if (vkCreateCommandPool(current_device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
	{
		spdlog::critical("[CommandList]: Failed to create command pool!");
		assert(false);
	}
}

void create_command_buffers()
{
	command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

	if (vkAllocateCommandBuffers(current_device, &alloc_info, command_buffers.data()) != VK_SUCCESS)
	{
		spdlog::critical("[CommandList]: Failed to create command buffer!");
		assert(false);
	}
}

void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index) 
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS)
	{
		spdlog::critical("[CommandList]: Failed to begin recording command buffer!");
		assert(false);
	}

	VkRenderPassBeginInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = swapchain_framebuffers[image_index];
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = swapchain_extent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clearColor;

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchain_extent.width);
	viewport.height = static_cast<float>(swapchain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain_extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);


	// TODO: make it only sort when you add stuff to the queu
	std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
		[](const sprite_queue_entry& lhs, const sprite_queue_entry& rhs) {
			return lhs.z < rhs.z;
		});

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sprite_graphics_pipeline);
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

		sprite_trigs_array[count + 0].position = { from_x, from_y, 0.0 };
		sprite_trigs_array[count + 1].position = { from_x, to_y, 0.0 };
		sprite_trigs_array[count + 2].position = { to_x, to_y, 0.0 };
		sprite_trigs_array[count + 3].position = { to_x, to_y, 0.0 };
		sprite_trigs_array[count + 4].position = { to_x, from_y, 0.0 };
		sprite_trigs_array[count + 5].position = { from_x, from_y, 0.0 };

		sprite_trigs_array[count + 0].texture = { from_u,	to_v };
		sprite_trigs_array[count + 1].texture = { from_u,	from_v };
		sprite_trigs_array[count + 2].texture = { to_u,		from_v };
		sprite_trigs_array[count + 3].texture = { to_u,		from_v };
		sprite_trigs_array[count + 4].texture = { to_u,		to_v };
		sprite_trigs_array[count + 5].texture = { from_u,	to_v };

		for (int i = 0; i < 6; ++i)
		{
			sprite_trigs_array[count + i].add_color = add_color;
			sprite_trigs_array[count + i].mul_color = mul_color;
		}

		vec3 anchor((to_x - from_x) * 0.5f, (to_y - from_y) * 0.5f, 0.0f);
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center))
		{
			for (int i = 0; i < 6; i++)
				sprite_trigs_array[count + i].position -= anchor;
		}

		if (spe.rotation != 0.0)
		{
			for (int i = 0; i < 6; i++)
				rotate_vector3d(sprite_trigs_array[count + i].position, (float)spe.rotation);
		}

		for (int i = 0; i < 6; i++)
		{
			sprite_trigs_array[count + i].position.x += (float)spe.x;
			sprite_trigs_array[count + i].position.y += (float)spe.y;
		}
		count += 6;

		bool render_batch = false;
		if (i < sprite_queue.size() - 1)
		{
			const auto& nspe = sprite_queue[i + 1];
			const auto& nsprite = sprites[nspe.sprite_id];
			render_batch = nsprite.image_id != sprite.image_id;
		}
		else
		{
			render_batch = true;
		}

		if (render_batch)
		{
			auto& value = textures.find(static_cast<const uint32_t&>(image.string_id))->second.data;
			value.upload_data(&sprite_trigs_array[0]);
			vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sprite_pipeline_layout, 1, 1, &textures.find(static_cast<const uint32_t&>(image.string_id))->second.descriptor_set, 0, nullptr);
			vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, sprite_pipeline_layout, 0, 1, &descriptor_sets[current_frame], 0, nullptr);
			vkCmdBindVertexBuffers(command_buffer, 0, 1, &value.buffer_data, offsets);
			vkCmdDraw(command_buffer, count, 1, 0, 0);
			count = 0;
		}
	}


	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_graphics_pipeline);
	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer_triangles.buffer_data, offsets);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline_layout, 0, 1, &descriptor_sets[current_frame], 0, nullptr);
	vkCmdDraw(command_buffer, triangles_count * 3, 1, 0, 0);

	vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer_lines.buffer_data, offsets);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline_layout, 0, 1, &descriptor_sets[current_frame], 0, nullptr);
	vkCmdDraw(command_buffer, lines_count * 2, 1, 0, 0);
}

void create_sync_objects() 
{
	image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		if (vkCreateSemaphore(current_device, &semaphoreInfo, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(current_device, &semaphoreInfo, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
			vkCreateFence(current_device, &fenceInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
		{
			spdlog::critical("[CommandList]: Failed to create synchronization objects for a frame!");
			assert(false);
		}
	}
}

void create_descriptor_pool() 
{
	const int max_texture = 1000;
	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = max_texture;

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) + max_texture;

	if (vkCreateDescriptorPool(current_device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.pImmutableSamplers = nullptr;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding sampler_layout_binding{};
	sampler_layout_binding.binding = 0;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = 1;
	layout_info.pBindings = &ubo_layout_binding;

	if (vkCreateDescriptorSetLayout(current_device, &layout_info, nullptr, &ubo_descriptor_set_layout) != VK_SUCCESS)
	{
		log::error("[Descriptor]: Failed to create descriptor set layout!");
		assert(false);
	}

	layout_info.pBindings = &sampler_layout_binding;

	if (vkCreateDescriptorSetLayout(current_device, &layout_info, nullptr, &texture_descriptor_set_layout) != VK_SUCCESS)
	{
		log::error("[Descriptor]: Failed to create texture descriptor set layout!");
		assert(false);
	}
}

void create_descriptor_sets() 
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, ubo_descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	alloc_info.pSetLayouts = layouts.data();

	descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(current_device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
	{
		spdlog::error("[Descriptor]: Failed to allocate descriptor sets!");
		assert(false);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = uniform_buffers[i].buffer_data;
		buffer_info.offset = 0;
		buffer_info.range = sizeof(uniform_vertex);

		VkWriteDescriptorSet descriptor_writes{};
		descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes.dstSet = descriptor_sets[i];
		descriptor_writes.dstBinding = 0;
		descriptor_writes.dstArrayElement = 0;
		descriptor_writes.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes.descriptorCount = 1;
		descriptor_writes.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(current_device, 1, &descriptor_writes, 0, nullptr);
	}
}

void create_uniform_buffers()
{
	VkDeviceSize bufferSize = sizeof(uniform_vertex);

	uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		auto& uniform = uniform_buffers[i];
		uniform.initialize(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vkMapMemory(current_device, uniform.buffer_memory, 0, bufferSize, 0, &uniform_buffers_mapped[i]);
	}
}

void update_uniform_buffer()
{
	auto w = width * 0.5f;
	auto h = height * 0.5f;
	mat4 p = ortho(-w, w, h, -h, -100.0f, 100.0f);
	mat4 v = lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 vp = p * v;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		uniform_vertex ubo{};
		ubo.view_projection_matrix = vp;

		memcpy(uniform_buffers_mapped[i], &ubo, sizeof(ubo));
	}
}

void switch_frame()
{
	vkWaitForFences(current_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(xs::render::internal::current_device, xs::render::internal::swapchain, UINT64_MAX, xs::render::internal::image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		swapchain_rebuild = true;
		return;
	}

	vkResetFences(current_device, 1, &in_flight_fences[current_frame]);

	vkResetCommandBuffer(command_buffers[current_frame], /*VkCommandBufferResetFlagBits*/ 0);
	record_command_buffer(command_buffers[current_frame], image_index);
}

void xs::render::initialize()
{
	width = configuration::width();
	height = configuration::height();

	create_instance();

#if defined(DEBUG)
	VkDebugUtilsMessengerCreateInfoEXT create_debug_info{};
	create_debug_handler(create_debug_info);

	if (create_debug_utils(instance, &create_debug_info, nullptr, &debug_messenger) != VK_SUCCESS)
	{
		spdlog::critical("[ValidationLayer]: Failed to set up debug messenger!");
		assert(false);
	}
#endif

	create_surface();
	pick_gpu_device();
	create_logical_device();
	create_present_queue();
	create_swapchain();
	create_image_views();
	create_render_pass();
	create_descriptor_set_layout();
	create_sprite_graphics_pipeline();
	create_graphics_pipeline();
	create_frame_buffers();
	create_command_pool();
	create_uniform_buffers();
	create_vertex_buffer();
	create_descriptor_pool();
	create_descriptor_sets();
	create_command_buffers();
	create_sync_objects();
	update_uniform_buffer();
}

void xs::render::render()
{
	XS_PROFILE_SECTION("xs::render::render");

	update_vertex_buffer();
	switch_frame();
}

void xs::render::shutdown()
{
	switch_frame();
	vkDeviceWaitIdle(current_device);

	for (auto framebuffer : swapchain_framebuffers)
		vkDestroyFramebuffer(current_device, framebuffer, nullptr);

	for (auto image_view : swapchain_image_views)
		vkDestroyImageView(current_device, image_view, nullptr);

	vkDestroySwapchainKHR(current_device, swapchain, nullptr);
	vertex_buffer_triangles.shutdown();
	vertex_buffer_lines.shutdown();

	vkDestroyPipeline(current_device, triangle_graphics_pipeline, nullptr);
	vkDestroyPipeline(current_device, sprite_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(current_device, triangle_pipeline_layout, nullptr);
	vkDestroyPipelineLayout(current_device, sprite_pipeline_layout, nullptr);
	vkDestroyRenderPass(current_device, render_pass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		uniform_buffers[i].shutdown();
		vkDestroySemaphore(current_device, render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(current_device, image_available_semaphores[i], nullptr);
		vkDestroyFence(current_device, in_flight_fences[i], nullptr);
	}

	vkDestroyDescriptorPool(current_device, descriptor_pool, nullptr);

	vkDestroyDescriptorSetLayout(current_device, ubo_descriptor_set_layout, nullptr);
	vkDestroyDescriptorSetLayout(current_device, texture_descriptor_set_layout, nullptr);

	vkDestroyCommandPool(current_device, command_pool, nullptr);
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
    VkFormat format;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImageTiling tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;

	VkFormatProperties fp;

	switch (img.channels)
	{
	case 1:
		format = VkFormat::VK_FORMAT_R8_SRGB;
		break;
	case 3:
		format = VkFormat::VK_FORMAT_R8G8B8_SRGB;
		break;
	case 4:
		format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		break;
	default:
		assert(false);
	}

	vkGetPhysicalDeviceFormatProperties(current_gpu, VkFormat::VK_FORMAT_R8G8B8_SRGB, &fp);

	if ((fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
	{
		log::warn("channels: {}, format type: VK_FORMAT_R8G8B8_SRGB not supported", img.channels);

		format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
	}

	texture tex;
	tex.initialize(data, img.width, img.height, tiling, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, format);
	tex.data.initialize(sizeof(sprite_trigs_array[0]) * (static_cast<unsigned long long>(sprite_trigs_max) * 3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	textures.insert(std::make_pair(static_cast<const uint32_t&>(img.string_id), tex));
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

static float inverseColorValue = 1.f / 255.0f;
void xs::render::set_color(color c)
{
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

void xs::render::transition_image_layout(const VkImage& image, const VkFormat& format, const VkImageLayout& oldLayout, const VkImageLayout& newLayout)
{
	VkCommandBuffer commandBuffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	end_single_time_commands(commandBuffer);
}

void device::swap_buffers()
{
	vkCmdEndRenderPass(command_buffers[current_frame]);

	if (vkEndCommandBuffer(command_buffers[current_frame]) != VK_SUCCESS)
	{
		spdlog::critical("[CommandList]: Failed to record command buffer!");
		assert(false);
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { image_available_semaphores[current_frame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command_buffers[current_frame];

	VkSemaphore signalSemaphores[] = { render_finished_semaphores[current_frame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	auto error = vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fences[current_frame]);
	if (error != VK_SUCCESS)
	{
		spdlog::critical("[CommandList]: Failed to submit draw command buffer!");
		assert(false);
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &image_index;
	vkQueuePresentKHR(present_queue, &presentInfo);

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	XS_PROFILE_FUNCTION();
}

VkDevice& xs::render::get_device()
{
	return xs::render::internal::current_device;
}

VkPhysicalDevice& xs::render::get_gpu_device()
{
	return current_gpu;
}

VkInstance& xs::render::get_instance()
{
	return instance;
}

VkQueue& xs::render::get_queue()
{
	return graphics_queue;
}

uint32_t xs::render::get_family_queue()
{
	return pick_queue_families(current_gpu).graphics_family.value();
}

VkSurfaceKHR& xs::render::get_surface()
{
	return surface;
}

VkRenderPass& xs::render::get_renderpass()
{
	return render_pass;
}

VkCommandBuffer& xs::render::get_command_buffer()
{
	return command_buffers[current_frame];
}

VkSwapchainKHR& xs::render::get_swapchain()
{
	return swapchain;
}

VkCommandBuffer xs::render::begin_single_time_commands()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(current_device, &alloc_info, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void xs::render::end_single_time_commands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(current_device, command_pool, 1, &command_buffer);
}

VkDescriptorPool& xs::render::get_descriptor_pool()
{
	return descriptor_pool;
}

VkDescriptorSetLayout& xs::render::get_texture_descriptor_set_layout()
{
	return texture_descriptor_set_layout;
}
#endif