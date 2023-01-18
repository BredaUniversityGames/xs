#pragma once
#include "stage_buffer_vk.h"
namespace xs::render
{
	class texture
	{
	public:
		void initialize(void* data, const uint32_t width, const uint32_t height, const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, const VkFormat format);
		void bind_texture();
		void create_texture_sampler();
		void create_texture_view(const VkFormat& format);
		void create_descriptor_sets();
		void shutdown();
		VkDescriptorSet descriptor_set;
	private:
		buffer buffer;
		stage_buffer stage_buffer;
		VkImage image;
		VkSampler texture_sampler;
		VkImageView image_view;
	};
}