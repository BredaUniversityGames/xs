#pragma once
#include <vulkan/vulkan.h>
namespace xs::render
{
	struct buffer
	{
		buffer() = default;
		void initialize(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties);
		void set_device_memory(const VkMemoryRequirements memory_requirement, const VkMemoryPropertyFlags properties);
		void upload_data(void* data);
		void shutdown();
		VkBuffer buffer_data;
		VkDeviceMemory buffer_memory;
		VkDeviceSize buffer_size;
	};
}
