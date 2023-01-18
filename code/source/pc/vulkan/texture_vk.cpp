#include "vulkan/render_vk.h"
#include "vulkan/texture_vk.h"
#include "log.h"

void xs::render::texture::initialize(void* data, const uint32_t width, const uint32_t height, const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, const VkFormat format)
{
    VkDevice& device = xs::render::get_device();

    create_texture_sampler();
    create_texture_view(format);
    create_descriptor_sets();

    stage_buffer.initialize(width * height * 4, data);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        log::error("failed to create image!");

    VkMemoryRequirements memory_requirement;
    vkGetImageMemoryRequirements(device, image, &memory_requirement);

    buffer.set_device_memory(memory_requirement, properties);

    vkBindImageMemory(device, image, buffer.buffer_memory, 0);

    xs::render::transition_image_layout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    stage_buffer.copy_buffer_to_image(stage_buffer.staging_buffer.buffer_data, image, width, height);
    xs::render::transition_image_layout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stage_buffer.staging_buffer.shutdown();
}

void xs::render::texture::bind_texture()
{
}

void xs::render::texture::create_texture_sampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(xs::render::get_gpu_device(), &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(xs::render::get_device(), &samplerInfo, nullptr, &texture_sampler) != VK_SUCCESS)
        log::error("failed to create texture sampler!");

}

void xs::render::texture::create_texture_view(const VkFormat& format)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(xs::render::get_device(), &viewInfo, nullptr, &image_view) != VK_SUCCESS)
        log::error("failed to create texture image view!");
}

void xs::render::texture::create_descriptor_sets()
{
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = xs::render::get_descriptor_pool();
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &xs::render::get_texture_descriptor_set_layout();

    if (vkAllocateDescriptorSets(xs::render::get_device(), &alloc_info, &descriptor_set) != VK_SUCCESS)
    {
        spdlog::error("[Descriptor]: Failed to allocate descriptor sets!");
        assert(false);
    }

    VkDescriptorImageInfo image_info{};
    image_info.sampler = texture_sampler;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //image_info.imageView = texture_image_view;

    VkWriteDescriptorSet descriptorWrites{};

    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = descriptor_set;
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pImageInfo = &image_info;

    vkUpdateDescriptorSets(xs::render::get_device(), 1, &descriptorWrites, 0, nullptr);
}

void xs::render::texture::shutdown()
{
    vkDestroySampler(xs::render::get_device(), texture_sampler, nullptr);
    vkDestroyImageView(xs::render::get_device(), image_view, nullptr);
}