#pragma once
#include <vulkan.hpp>
namespace xs::render::fence
{
	void initialize();
	void shutdown();

	VkFenceCreateInfo get();
}