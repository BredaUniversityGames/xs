#pragma once
#include "buffer_vk.h"
namespace xs::render
{
	struct stage_buffer
	{
		stage_buffer() = default;
		void initialize(const VkDeviceSize& size, void* data);
		void copy_buffer(const VkBuffer& source, const VkBuffer& destination, const VkDeviceSize& size);
		void copy_buffer_to_image(const VkBuffer& source, const VkImage& image, const uint32_t width, const uint32_t height);
		void shutdown();
		buffer buffer_;
		buffer staging_buffer;
	};
}
