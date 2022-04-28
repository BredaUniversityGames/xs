#pragma once

#include "imgui.h"

IMGUI_API bool ImGui_Impl_PS5_Init();
IMGUI_API void ImGui_Impl_PS5_Shutdown();
IMGUI_API void ImGui_Impl_PS5_NewFrame();
IMGUI_API void ImGui_Impl_PS5_RenderDrawData(ImDrawData* data);
