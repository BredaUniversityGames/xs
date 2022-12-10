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

bool ImGui_Impl_Init()
{
	//const auto window = xs::device::get_window();
	//const bool glfw = ImGui_ImplGlfw_InitForOpenGL(window, true);
	//return glfw;
	return true;
}

void ImGui_Impl_Shutdown()
{
	//ImGui_ImplGlfw_Shutdown();
}

void ImGui_Impl_NewFrame()
{
	//ImGui_ImplGlfw_NewFrame();
	//ImGui::NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData* draw_data)
{
	//ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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