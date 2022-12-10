#pragma once
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
}