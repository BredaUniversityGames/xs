#include "inspector.h"

#ifdef INSPECTOR
#include <imgui.h>
#include <imgui_impl.h>
#include <imgui_internal.h>
#include <implot.h>
#include "IconsFontAwesome5.h"
#include "fileio.h"
#include "script.h"
#include "data.h"
#include "log.h"
#include "configuration.h"
#include "../include/version.h"
#include "profiler.h"
#include "device.h"
#include "render.h"
#include "tools.h"
#include "render_internal.h"
#if defined(PLATFORM_PC)
#include <GLFW/glfw3.h>
#include "device_pc.h"
#elif defined(PLATFORM_SWITCH)
#include <nn/fs.h>
#endif

namespace xs::inspector
{
	struct notification
	{
		int id;
		notification_type type;
		std::string message;
		float time;
	};

	bool game_paused = false;
	float ui_scale = 1.0f;
	bool show_registry = false;
	bool show_profiler = false;
	bool show_about = false;
	bool show_demo = false;
	void embrace_the_darkness();
	void follow_the_light();
	void go_gray();
	float ok_timer = 0.0f;
	bool theme = false;
	bool next_frame;
	std::vector<notification> notifications;

	void update_notifications();
}

using namespace xs::inspector;
using namespace xs;
using namespace std;

void xs::inspector::initialize()
{
    ImGui::CreateContext();
#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH) || defined(PLATFORM_APPLE)
	ImPlot::CreateContext();
#endif
    ImGui_Impl_Init();
	
#if defined(PLATFORM_PC)
	float ys;
	float xs;
	glfwGetWindowContentScale(device::get_window(), &xs, &ys);
	ui_scale = (xs + ys) / 2.0f;
#endif

	
#if defined(DEBUG) && defined(PLATFORM_PC)
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#else	
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
#endif

	ImGuiIO& io = ImGui::GetIO();

	const float UIScale = ui_scale;
	const float fontSize = 14.0f;
	const float iconSize = 12.0f;

	ImFontConfig config;
	config.OversampleH = 8;
	config.OversampleV = 8;
		
	io.Fonts->AddFontFromFileTTF(fileio::get_path("[games]/shared/fonts/DroidSans.ttf").c_str(), fontSize * UIScale, &config);

	static const ImWchar letter_ranges[] = { 0x2000, 0x206F, 0 }; // will not be copied by AddFont* so keep in scope.
	config.MergeMode = true;
	io.Fonts->AddFontFromFileTTF(fileio::get_path("[games]/shared/fonts/DroidSans.ttf").c_str(), fontSize * UIScale, &config, letter_ranges);

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

	xs::inspector::go_gray();
}

void xs::inspector::shutdown()
{
	ImGui_Impl_Shutdown();
#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH) || defined(PLATFORM_APPLE)
	ImPlot::DestroyContext();
#endif
	ImGui::DestroyContext();
}

void Tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.6f)
	{
		ImGui::BeginTooltip();
		ImGui::SetTooltip("%s", tooltip);
		ImGui::EndTooltip();
	}
}

void xs::inspector::render(float dt)
{
#ifdef PLATFORM_PS5
	return;
#endif
	
	ImGui_Impl_NewFrame();
    ImGui::NewFrame();
		
	ok_timer -= dt;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.31f, 0.31f, 0.31f, 0.21f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.23f, 0.23f, 0.00f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	
    auto mousePos = ImGui::GetMousePos();
	if (xs::script::has_error() ||
		show_registry ||
		show_profiler ||
		show_about	||
		show_demo		||
		(ImGui::IsMousePosValid() &&
         mousePos.y < 100.0f &&
         mousePos.y >= 0.0f &&
         mousePos.y <= device::get_height() &&
         mousePos.x >= 0.0f &&
         mousePos.x <= device::get_width()))
	{				
		bool true_that = true;
		ImGui::Begin("Window", &true_that,
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetWindowPos({ 0, 0 });
		ImGui::SetWindowSize({-1, -1 });
        
		if (ImGui::Button(ICON_FA_SYNC_ALT))
		{		
			script::shutdown();
			script::configure();
			script::initialize();
			if(!xs::script::has_error())
				ok_timer = 4.0f;
		}		
		Tooltip("Reload Game");		
	
		ImGui::SameLine();
		if (game_paused)
		{
			if (ImGui::Button(ICON_FA_PLAY))
				game_paused = false;
			Tooltip("Play");			
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_FAST_FORWARD))
				next_frame = true;
			Tooltip("Next Frame");			
		}
		else
		{
			if (ImGui::Button(ICON_FA_PAUSE))
				game_paused = true;
			Tooltip("Pause");
		}		

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_DATABASE))
		{
			show_registry = !show_registry;
		}
		Tooltip("Data");
		
		
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_CHART_BAR))
		{
			show_profiler = !show_profiler;
		}
		Tooltip("Profiler");

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_IMAGES))
		{
			render::reload_images();	
			next_frame = true;
		}
		Tooltip("Reload Art");

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_ADJUST))
		{
			theme = !theme;
			if (theme)
				embrace_the_darkness();				
			else
				go_gray();
				
		}
		Tooltip("Theme");
		
		
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_LIFE_RING))
		{
			show_demo = !show_demo;
		}
		Tooltip("Demo Window");
						
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_QUESTION_CIRCLE))
		{
			show_about = true;
		}
		Tooltip("About");
		
		ImGui::SameLine();
		ImGui::Text("  ");

		// Make hover buttons transparent
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.23f, 0.23f, 0.23f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.23f, 0.23f, 0.23f, 1.00f));		

		{ // Memory usage
			auto mb = script::get_bytes_allocated() / (1024.0f * 1024.0f);
			auto mem_str = ICON_FA_MICROCHIP + string(" ") + xs::tools::float_to_str_with_precision(mb, 1) + string("##memory");
			ImGui::SameLine();
			ImGui::Button(mem_str.c_str());			
			Tooltip("VM Memory Usage");
		}


		{ // Render stats
			ImGui::SameLine();
			auto& stats = render::get_stats();
			auto draw_calls = ICON_FA_PAINT_BRUSH + string(" ") + to_string(stats.draw_calls); // +string(" ");
			auto sprites = ICON_FA_SQUARE + string(" ") + to_string(stats.sprites); // +string(" ");
			auto textures = ICON_FA_IMAGE + string(" ") + to_string(stats.textures); // +string(" ");

			ImGui::Button(draw_calls.c_str());
			Tooltip("Draw Calls");
			ImGui::SameLine();

			ImGui::Button(sprites.c_str());
			Tooltip("Sprites");
			ImGui::SameLine();

			ImGui::Button(textures.c_str());
			Tooltip("Textures");			
		}
		
		{ // Version
			ImGui::SameLine();
			string version_string = ICON_FA_TAGS + string(" ") + xs::version::version_string;
			ImGui::PushID("version");
			ImGui::Button(version_string.c_str());
			ImGui::PopID();
			Tooltip("Version");		
		}

		ImGui::PopStyleColor(2);

		ImGui::SameLine();
		if (xs::script::has_error())
		{
			game_paused = true;
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.33f, 0.33f, 1.0f));
			if (ImGui::Button(ICON_FA_TIMES_CIRCLE))
			{
				xs::script::clear_error();
				game_paused = false;
			}
			Tooltip("Script Error! Check output.");
			ImGui::PopStyleColor(1);
		}

		if (xs::data::has_chages()) {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFF5FB9);
			if (ImGui::Button(ICON_FA_EXCLAMATION_TRIANGLE)) {
				show_registry = true;
			}
			ImGui::PopStyleColor();
			Tooltip("Data has unsaved changes");
		}
		
		// ImGui::PopStyleColor();

		ImGui::End();		
	}

	{
		float x = -10.0f;
		float y = -10.0f;
		for (auto& n : notifications)
		{
			static bool true_that2 = true;
			const auto& io = ImGui::GetIO();  
			ImGui::Begin(to_string(n.id).c_str(), &true_that2,
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::SetWindowPos({ io.DisplaySize.x - ImGui::GetWindowWidth() + x, io.DisplaySize.y - ImGui::GetWindowHeight() + y });
			// Draw the notification icon

			ImVec4 color;
			string icon;
			switch (n.type)
			{
			case notification_type::info:
				color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				icon = ICON_FA_INFO_CIRCLE;
				break;
			case notification_type::success:
				color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				icon = ICON_FA_CHECK_CIRCLE;
				break;
			case notification_type::warning:
				color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
				icon = ICON_FA_EXCLAMATION_TRIANGLE;
				break;
			case notification_type::error:
				color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
				icon = ICON_FA_TIMES_CIRCLE;
				break;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Button(icon.c_str());			
			ImGui::PopStyleColor(1);
			ImGui::SameLine();
			ImGui::Text(n.message.c_str());				
			y -= ImGui::GetWindowHeight() + 10.0f;
			ImGui::End();
			n.time -= dt;
		}

		notifications.erase(
			std::remove_if(
				notifications.begin(),
				notifications.end(),
				[](const notification& n) { return n.time <= 0.0f; }),
			notifications.end());
	}

	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(2);
	
	if (show_registry)
	{
		xs::data::inspect(show_registry);
	}

	if (show_profiler)
	{
		xs::profiler::inspect(show_profiler);
	}
	
	if (show_about)
	{
		ImGui::Begin("About", &show_about, ImGuiWindowFlags_Modal);
		ImGui::Text(" xs %s ", xs::version::version_string.c_str());
		ImGui::Text(" Made with love at Breda University of Applied Sciences ");
		ImGui::End();
	}
	
	if(show_demo)
	{
		ImGui::ShowDemoWindow();
	}
	
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
	bool t = game_paused && !next_frame;
	next_frame = false;
	return t;
}

int xs::inspector::notify(notification_type type, const std::string& message, float time)
{
	static int id = 0;
	notifications.push_back({ id, type, message, time });
	return id++;
}

void xs::inspector::clear_notification(int id)
{
	notifications.erase(
		std::remove_if(
			notifications.begin(),
			notifications.end(),
			[id](const notification& n) { return n.id == id; }),
		notifications.end());
}

void xs::inspector::embrace_the_darkness()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.80f, 0.80f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	//colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	//colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
	// style.CellPadding = ImVec2(6.00f, 6.00f);
	style.ItemSpacing = ImVec2(6.00f, 6.00f);
	style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 15;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 1;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.FrameBorderSize = 1;
	style.TabBorderSize = 1;
	style.WindowRounding = 7;
	style.ChildRounding = 4;
	style.FrameRounding = 3;
	style.PopupRounding = 4;
	style.ScrollbarRounding = 9;
	style.GrabRounding = 3;
	style.LogSliderDeadzone = 4;
	style.TabRounding = 4;
	style.FrameBorderSize = 0;
}

void xs::inspector::follow_the_light()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
	colors[ImGuiCol_Button] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
	//colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.22f);
	//colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowBorderSize = 0;
	style.ChildBorderSize = 0;
	style.PopupBorderSize = 0;
	style.FrameBorderSize = 0;
	style.TabBorderSize = 0;
}

void xs::inspector::go_gray()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.24f, 0.24f, 0.24f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.54f, 0.54f, 0.54f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.22f, 0.22f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	//colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	//colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
	// style.CellPadding = ImVec2(6.00f, 6.00f);
	style.ItemSpacing = ImVec2(6.00f, 6.00f);
	style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 10;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 1;
	style.ChildBorderSize = 0;
	style.PopupBorderSize = 0;
	style.FrameBorderSize = 0;
	style.TabBorderSize = 1;
	style.WindowRounding = 7;
	style.ChildRounding = 4;
	style.FrameRounding = 3;
	style.PopupRounding = 4;
	style.ScrollbarRounding = 9;
	style.GrabRounding = 3;
	style.LogSliderDeadzone = 4;
	style.TabRounding = 4;
	style.FrameBorderSize = 0;
}

void xs::inspector::update_notifications()
{

}

#else

void xs::inspector::initialize() {}
void xs::inspector::shutdown() {}
void xs::inspector::render(float dt) {}
bool xs::inspector::paused() { return false; }

#endif
