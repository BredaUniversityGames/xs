#pragma once

#ifndef IMGUI_VERSION
#error Must include imgui.h before imgui_impl.h
#endif

IMGUI_API bool ImGui_Impl_Init();
IMGUI_API void ImGui_Impl_Shutdown();
IMGUI_API void ImGui_Impl_NewFrame();
IMGUI_API void ImGui_Impl_RenderDrawData(ImDrawData* draw_data);

// Platform dependent
#ifdef PLATFORM_PC
typedef union SDL_Event SDL_Event;
IMGUI_API void ImGui_Impl_ProcessEvent(const SDL_Event* event);
#endif // PLATFORM_PC

