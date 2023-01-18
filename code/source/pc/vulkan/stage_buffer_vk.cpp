#if defined(VULKAN)
#include <ios>
#include "vulkan/stage_buffer_vk.h"
#include "vulkan/render_vk.h"

void xs::render::stage_buffer::initialize(const VkDeviceSize& size, void* data)
{
    staging_buffer.initialize(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    staging_buffer.upload_data(data);

    buffer_.initialize(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buffer.buffer_data, buffer_.buffer_data, size);
}

void xs::render::stage_buffer::copy_buffer(const VkBuffer& source, const VkBuffer& destination, const VkDeviceSize& size)
{
    VkCommandBuffer command_buffer = xs::render::begin_single_time_commands();

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

    xs::render::end_single_time_commands(command_buffer);
}

void xs::render::stage_buffer::copy_buffer_to_image(const VkBuffer& source, const VkImage& image, const uint32_t width, const uint32_t height)
{
    VkCommandBuffer command_buffer = xs::render::begin_single_time_commands();

    VkBufferImageCopy copy_region{};
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;
    copy_region.imageOffset = { 0, 0, 0 };
    copy_region.imageExtent = {
        width,
        height,
        1
    };
    vkCmdCopyBufferToImage(command_buffer, source, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    xs::render::end_single_time_commands(command_buffer);
}

void xs::render::stage_buffer::shutdown()
{
    buffer_.shutdown();
}
#endif