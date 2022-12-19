#pragma once
#include "buffer_vk.h"
namespace xs::render
{
	struct stage_buffer
	{
		stage_buffer() = default;
		void initialize(VkDeviceSize size, void* data);
		void copy_buffer(VkBuffer& source, VkBuffer& destination,VkDeviceSize size);
		void shutdown();
		buffer buffer_;
	};
}
