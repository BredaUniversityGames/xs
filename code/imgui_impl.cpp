#include "imgui.h"
#include "imgui_impl.h"

#if defined(PLATFORM_PC)
#include <SDL3/SDL.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "device.hpp"
#include "device_sdl.hpp"
#include "opengl.hpp"



bool ImGui_Impl_Init()
{
	const char* glsl_version = "#version 130";
	const auto window = xs::device::get_window();
	auto gl_context = xs::device::get_context();
	const bool sdl = ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
	const bool opengl = ImGui_ImplOpenGL3_Init(glsl_version);
	return sdl && opengl;
}

void ImGui_Impl_Shutdown()
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
}

void ImGui_Impl_NewFrame()
{	
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ImGui_ImplOpenGL3_RenderDrawData(draw_data);

	// Update and Render additional Platform Windows
	auto io = ImGui::GetIO();
	if (io.ConfigFlags)
	{
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}
}

IMGUI_API void ImGui_Impl_ProcessEvent(const SDL_Event* event)
{
#ifdef INSPECTOR	
	ImGui_ImplSDL3_ProcessEvent(event);
#endif
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

#elif defined(PLATFORM_APPLE)

#include <SDL3/SDL.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_metal.h"
#include "device_apple.h"
#include "device.hpp"

using namespace xs;

bool ImGui_Impl_Init()
{
    SDL_Window* window = device::get_window();
    id<MTLDevice> device = device::internal::get_device();
    
    bool sdl = ImGui_ImplSDL3_InitForMetal(window);
    bool metal = ImGui_ImplMetal_Init(device);
    assert(sdl);
    assert(metal);
    return sdl && metal;
}

void ImGui_Impl_Shutdown()
{
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplMetal_Shutdown();
}

void ImGui_Impl_NewFrame()
{
    @autoreleasepool {
    // SDL3 backend sets DisplaySize from the window - always call this
    ImGui_ImplSDL3_NewFrame();

    // Get the current drawable - it was acquired in begin_frame()
    id<CAMetalDrawable> drawable = device::internal::get_current_drawable();

    // Create a render pass descriptor for ImGui Metal backend
    // This is required for ImGui to know the framebuffer format
    MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];

    if (drawable)
    {
        rpd.colorAttachments[0].texture = drawable.texture;
    }
    else
    {
        // No drawable available - use the layer's pixel format with a dummy texture
        // This can happen on first frame or when window is minimized
        CAMetalLayer* layer = device::internal::get_metal_layer();
        MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:layer.pixelFormat
                                                                                           width:1
                                                                                          height:1
                                                                                       mipmapped:NO];
        texDesc.usage = MTLTextureUsageRenderTarget;
        rpd.colorAttachments[0].texture = [device::internal::get_device() newTextureWithDescriptor:texDesc];
    }

    rpd.colorAttachments[0].loadAction = MTLLoadActionLoad;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

    ImGui_ImplMetal_NewFrame(rpd);
    }
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
    ImGui_ImplMetal_RenderDrawData(draw_data,
                                   device::internal::get_command_buffer(),
                                   device::internal::get_render_encoder());
}

IMGUI_API void ImGui_Impl_ProcessEvent(const SDL_Event* event)
{
#ifdef INSPECTOR
    ImGui_ImplSDL3_ProcessEvent(event);
#endif
}

#endif
