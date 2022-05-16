#include "inspector.h"
#include <imgui.h>
#include <imgui_impl.h>
#include "IconsFontAwesome5.h"
#include "fileio.h"
#include "script.h"
#if defined(PLATFORM_PC)
#include <GLFW/glfw3.h>
#include "device_pc.h"
#elif defined(PLATFORM_SWITCH)
#include <nn/fs.h>
#endif


namespace xs::inspector::internal
{
	bool paused = false;
	float ui_scale = 1.0f;
}

void xs::inspector::initialize()
{
	ImGui::CreateContext();
	ImGui_Impl_Init();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#if defined(PLATFORM_PC)
	float ys;
	float xs;
	glfwGetWindowContentScale(device::get_window(), &xs, &ys);
	internal::ui_scale = (xs + ys) / 2.0f;
#endif

	
#if defined(DEBUG) && defined(PLATFORM_PC)
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#else	
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
#endif

	ImGuiIO& io = ImGui::GetIO();

	const float UIScale = internal::ui_scale;
	const float fontSize = 14.0f;
	const float iconSize = 12.0f;

	ImFontConfig config;
	config.OversampleH = 8;
	config.OversampleV = 8;
	io.Fonts->AddFontFromFileTTF(fileio::get_path("[games]/shared/fonts/DroidSans.ttf").c_str(),
		fontSize * UIScale,
		&config);
	static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.
	config.MergeMode = true;
	config.OversampleH = 8;
	config.OversampleV = 8;

	std::string fontpath = fileio::get_path("[games]/shared/fonts/FontAwesome5FreeSolid900.otf");
	io.Fonts->AddFontFromFileTTF(fontpath.c_str(), iconSize * UIScale, &config, icons_ranges);

	const std::string iniPath = fileio::get_path("[save]/imgui.ini");
	const char* constStr = iniPath.c_str();
	char* str = new char[iniPath.size() + 1];
	strcpy(str, constStr);
	io.IniFilename = str;
}

void xs::inspector::shutdown()
{
	ImGui_Impl_Shutdown();
	ImGui::DestroyContext();
}

void xs::inspector::render(float dt)
{
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);	// TODO: Only OpenGL call

	ImGui_Impl_NewFrame();
	// ^^^ Move the bind bind here!


	bool true_that = true;
	ImGui::SetWindowPos({ 0,0 });
	ImGui::Begin("Window", &true_that, 
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::SetWindowPos({ 0,0 });

	// const ImVec2 btnSize(24.0f, 24.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * internal::ui_scale);	
	if(ImGui::Button(ICON_FA_SYNC_ALT))
	{
		script::shutdown();
		script::initialize(nullptr);
	}
	ImGui::SameLine();
	if(internal::paused)
	{
		if (ImGui::Button(ICON_FA_PLAY))
			internal::paused = false;
	}
	else
	{
		if (ImGui::Button(ICON_FA_PAUSE))
			internal::paused = true;
	}
	ImGui::SameLine();
	ImGui::Text("dt=%f", dt);
	
	ImGui::PopStyleVar();
	
	ImGui::End();		

	ImGui::Render();
	ImGui_Impl_RenderDrawData(ImGui::GetDrawData());

#if defined(PLATFORM_SWITCH)
	// Commit updated content to the specified mount name
	// Make sure the content is not in <tt>nn::fs::OpenMode_Write</tt>
	nn::fs::Commit("save");
#endif
}

bool xs::inspector::paused()
{
	return internal::paused;
}
