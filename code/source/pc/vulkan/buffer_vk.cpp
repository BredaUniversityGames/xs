#if defined(VULKAN)
#include "vulkan/buffer_vk.h"
#include "vulkan/render_vk.h"
#include <ios>
#include "log.h"
namespace xs::render
{
    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(xs::render::get_gpu_device(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        spdlog::error("[Memory]: failed to find suitable memory type!");
        assert(false);
        return 0;
    }

	void buffer::initialize(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties)
	{
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        buffer_size = size;

        if (vkCreateBuffer(xs::render::get_device(), &buffer_info, nullptr, &buffer_data) != VK_SUCCESS) 
        {
            spdlog::error("[Buffer]: could not create buffer!");
            assert(false);
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(xs::render::get_device(), buffer_data, &mem_requirements);

        set_device_memory(mem_requirements, properties);

        vkBindBufferMemory(xs::render::get_device(), buffer_data, buffer_memory, 0);
	}

    void buffer::set_device_memory(const VkMemoryRequirements memory_requirement, const VkMemoryPropertyFlags properties)
    {
        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = memory_requirement.size;
        alloc_info.memoryTypeIndex = find_memory_type(memory_requirement.memoryTypeBits, properties);

        if (vkAllocateMemory(xs::render::get_device(), &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS)
        {
            spdlog::error("[Buffer]: failed to allocate buffer memory!");
            assert(false);
        }
    }

	void buffer::upload_data(void* data)
	{
        assert(static_cast<uint32_t>(buffer_size) >= 0);
        void* pointer = nullptr;
        vkMapMemory(xs::render::get_device(), buffer_memory, 0, buffer_size, 0, &pointer);
        memcpy(pointer, data, static_cast<size_t>(buffer_size));
        vkUnmapMemory(xs::render::get_device(), buffer_memory);
	}

	void buffer::shutdown()
	{
        vkDestroyBuffer(xs::render::get_device(), buffer_data, nullptr);
        vkFreeMemory(xs::render::get_device(), buffer_memory, nullptr);
	}

}
#endif