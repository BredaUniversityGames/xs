#include "imgui.h"
#include "imgui_impl.h"

#if defined(PLATFORM_PC)
#include <SDL3/SDL.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "device.hpp"
#include "device_pc.hpp"
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

#include "imgui_impl_osx.h"
#include "imgui_impl_metal.h"
#include "device_apple.h"


using namespace xs;

bool ImGui_Impl_Init()
{
    MTKView* view = device::internal::get_view();
    auto osx = ImGui_ImplOSX_Init(device::internal::get_view());
    auto metal = ImGui_ImplMetal_Init(view.device);
    assert(osx);
    assert(metal);
    return osx && metal;
}

void ImGui_Impl_Shutdown()
{
    ImGui_ImplOSX_Shutdown();
    ImGui_ImplMetal_Shutdown();
}

void ImGui_Impl_NewFrame()
{
    MTKView* view = device::internal::get_view();
    ImGui_ImplOSX_NewFrame(view);
    ImGui_ImplMetal_NewFrame(view.currentRenderPassDescriptor);
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
    ImGui_ImplMetal_RenderDrawData(draw_data,
                                   device::internal::get_command_buffer(),
                                   device::internal::get_render_encoder());
}

#endif
