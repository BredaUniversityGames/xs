#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>
const int MAX_FRAMES_IN_FLIGHT = 2;
namespace xs::render
{

	VkDevice& get_device();
	VkPhysicalDevice& get_gpu_device();
	VkInstance& get_instance();
	VkQueue& get_queue();
	uint32_t get_family_queue();
	VkSurfaceKHR& get_surface();
	VkRenderPass& get_renderpass();
	VkCommandBuffer& get_command_buffer();
	VkSwapchainKHR& get_swapchain();
	VkDescriptorPool& get_descriptor_pool();
	VkDescriptorSetLayout& get_texture_descriptor_set_layout();
    VkCommandBuffer begin_single_time_commands();
	void transition_image_layout(const VkImage& image, const VkFormat& format, const VkImageLayout& oldLayout, const VkImageLayout& newLayout);
	void end_single_time_commands(VkCommandBuffer command_buffer);
}