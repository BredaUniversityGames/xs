#include "imgui.h"
#include "imgui_impl.h"

#if defined(OPENGL)
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "device.h"
#include "device_pc.h"

bool ImGui_Impl_Init()
{
	const auto window = xs::device::get_window();
	const bool opengl = ImGui_ImplOpenGL3_Init();
	const bool glfw = ImGui_ImplGlfw_InitForOpenGL(window, true);	
	return glfw && opengl;
}

void ImGui_Impl_Shutdown()
{
	ImGui_ImplGlfw_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#elif defined(VULKAN)
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "device.h"
#include "device_pc.h"
#include "vulkan/render_vk.h"
#include "configuration.h"

ImGui_ImplVulkanH_Window vulkan_imgui_window;
VkDescriptorPool         descriptor_pool = VK_NULL_HANDLE;

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

bool ImGui_Impl_Init()
{
	const auto window = xs::device::get_window();
	const bool glfw = ImGui_ImplGlfw_InitForVulkan(window, true);
    auto wd = &vulkan_imgui_window;


    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        vkCreateDescriptorPool(xs::render::get_device(), &pool_info, nullptr, &descriptor_pool);
    }

    wd->Surface = xs::render::get_surface();
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = xs::render::get_instance();
    init_info.PhysicalDevice = xs::render::get_gpu_device();
    init_info.Device = xs::render::get_device();
    init_info.QueueFamily = xs::render::get_family_queue();
    init_info.Queue = xs::render::get_queue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, xs::render::get_renderpass());

    ImGui_ImplVulkan_CreateFontsTexture(xs::render::get_command_buffer());
	return glfw;
}

void ImGui_Impl_Shutdown()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	ImGui_ImplVulkanH_DestroyWindow(xs::render::get_instance(), xs::render::get_device(), &vulkan_imgui_window, nullptr);
	ImGui_ImplGlfw_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
    VkResult err;
    auto wd = &vulkan_imgui_window;
    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(xs::render::get_device(), wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(xs::render::get_device(), 1, &fd->Fence, VK_TRUE, UINT64_MAX); 

        err = vkResetFences(xs::render::get_device(), 1, &fd->Fence);
    }
    {
        err = vkResetCommandPool(xs::render::get_device(), fd->CommandPool, 0);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
}
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        err = vkQueueSubmit(xs::render::get_queue(), 1, &info, fd->Fence);
    }
}

#elif defined(PLATFORM_SWITCH)

#include "imgui_impl_switch.h"

bool ImGui_Impl_Init()
{
	return ImGui_Impl_Switch_Init();
}

void ImGui_Impl_Shutdown()
{
	ImGui_Impl_Switch_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_Impl_Switch_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_Impl_Switch_RenderDrawData(ImGui::GetDrawData());
}

#elif defined(PLATFORM_PS5)

#include "imgui_impl_PS5.h"

bool ImGui_Impl_Init()
{
	return ImGui_Impl_PS5_Init();
}

void ImGui_Impl_Shutdown()
{
	ImGui_Impl_PS5_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	ImGui_Impl_PS5_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	ImGui_Impl_PS5_RenderDrawData(ImGui::GetDrawData());
}

#endif