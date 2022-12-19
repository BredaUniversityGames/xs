#if defined(VULKAN)
#include <ios>
#include "vulkan/stage_buffer_vk.h"
#include "vulkan/render_vk.h"

void xs::render::stage_buffer::initialize(VkDeviceSize size, void* data)
{
    buffer stagingBuffer;
    stagingBuffer.initialize(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    stagingBuffer.upload_data(data);

    buffer_.initialize(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(stagingBuffer.buffer_data, buffer_.buffer_data, size);

    stagingBuffer.shutdown();
}

void xs::render::stage_buffer::copy_buffer(VkBuffer& source, VkBuffer& destination, VkDeviceSize size)
{
    VkCommandBuffer command_buffer = xs::render::begin_single_time_commands();

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

    xs::render::end_single_time_commands(command_buffer);
}

void xs::render::stage_buffer::shutdown()
{
    buffer_.shutdown();
}
#endif