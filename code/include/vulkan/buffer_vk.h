#pragma once
#include <vulkan/vulkan.h>
namespace xs::render
{
	struct buffer
	{
		buffer() = default;
		void initialize(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		void upload_data(void* data);
		void shutdown();
		VkBuffer buffer_data;
		VkDeviceMemory buffer_memory;
		VkDeviceSize buffer_size;
	};
}
