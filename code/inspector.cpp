#include "inspector.hpp"

#ifdef INSPECTOR
#include <imgui.h>
#include <imgui_impl.h>
#include <imgui_internal.h>
#include <implot.h>
#include "IconsFontAwesome5.h"
#include "fileio.hpp"
#include "script.hpp"
#include "data.hpp"
#include "log.hpp"
#include "configuration.hpp"
#include "version.hpp"
#include "profiler.hpp"
#include "device.hpp"
#include "render.hpp"
#include "tools.hpp"
#include "input.hpp"
#include "render_internal.hpp"
#include "packager.hpp"
#include "xs.hpp"

#ifdef EDITOR
#include "dialogs/portable-file-dialogs.h"
#endif

#if defined(PLATFORM_PC)
#include "device_pc.hpp"
#endif

#define SHOW_IMGUI_DEMO 0

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
	bool restart_flag = false;
	float ui_scale = 1.0f;
	bool show_registry = false;
	bool show_profiler = false;
	bool show_about = false;
	bool show_demo = false;
	bool show_modal = false;
	bool show_inspector = false;
	char* ini_filename = nullptr;

	enum class theme
	{
		dark = 0,
		light = 1,
		gray = 2,
		acrylic = 3
	};

	theme current_theme = theme::acrylic;
	void apply_theme();
	void push_menu_theme();
	void pop_menu_theme();
	void embrace_the_darkness();
	void follow_the_light();
	void go_gray();
	void see_through();

	double ok_timer = 0.0f;
	bool next_frame;
	std::vector<notification> notifications;
	ImFont* small_font = nullptr;
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
	
	auto& io = ImGui::GetIO();
#if defined(XS_DEBUG) && defined(PLATFORM_PC)
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#else
	io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
#endif

	const float UIScale = (float)device::hdpi_scaling();
	const float fontSize = 16.0f;
	const float iconSize = 12.0f;

	ImFontConfig config;
	config.OversampleH = 8;
	config.OversampleV = 8;
	
	auto selawk = fileio::get_path("[shared]/fonts/selawk.ttf");
	if(!fileio::exists(selawk))
	{
		log::critical("Could not find the font file selawk.ttf at path:{}", selawk);
		return;
	}
	auto selawk_data = fileio::read_binary_file(selawk);
	// Copy to a heap allocation to ensure it stays valid
	char* selawk_buffer = new char[selawk_data.size()];
	memcpy(selawk_buffer, selawk_data.data(), selawk_data.size());
	auto selawk_font = io.Fonts->AddFontFromMemoryTTF(selawk_buffer, (int)selawk_data.size(), fontSize * UIScale, &config);
	assert(selawk_font);
		
	static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; // will not be copied by AddFont* so keep in scope.
	config.MergeMode = true;

	std::string font_awesome = fileio::get_path("[shared]/fonts/FontAwesome5FreeSolid900.otf");
	if(!fileio::exists(font_awesome))
	{
		log::critical("Could not find the font file FontAwesome5FreeSolid900.otf at path:{}", font_awesome);
		return;
	}
	auto font_awesome_data = fileio::read_binary_file(font_awesome);
	// Copy to a heap allocation to ensure it stays valid
	char* font_awesome_buffer = new char[font_awesome_data.size()];
	memcpy(font_awesome_buffer, font_awesome_data.data(), font_awesome_data.size());
	auto font_awesome_font = io.Fonts->AddFontFromMemoryTTF(
		font_awesome_buffer,
		(int)font_awesome_data.size(),
		iconSize * UIScale,
		&config,
		icons_ranges);
	assert(font_awesome_font != nullptr);

	// Copy to a heap allocation (again) to ensure it stays valid
	selawk_buffer = new char[selawk_data.size()];
	memcpy(selawk_buffer, selawk_data.data(), selawk_data.size());
	small_font = io.Fonts->AddFontFromMemoryTTF(selawk_buffer, (int)selawk_data.size(), 12.0f * UIScale, nullptr);
	assert(small_font);


	const std::string iniPath = fileio::get_path("[user]/imgui.ini");
	const char* constStr = iniPath.c_str();
	ini_filename = new char[iniPath.size() + 1];
	strcpy(ini_filename, constStr);
	io.IniFilename = ini_filename;

	current_theme = (theme)data::get_number("theme", data::type::user);
	show_inspector = data::get_bool("show_inspector", data::type::user);
	apply_theme();
}

void xs::inspector::shutdown()
{
	ImGui_Impl_Shutdown();
#if defined(PLATFORM_PC) || defined(PLATFORM_SWITCH) || defined(PLATFORM_APPLE)
	ImPlot::DestroyContext();
#endif
	ImGui::DestroyContext();
	delete[] ini_filename;
}

static void tooltip(const char* tooltip)
{
	if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.6f)
	{
		ImGui::BeginTooltip();
		ImGui::SetTooltip("%s", tooltip);
		ImGui::EndTooltip();
	}	
}

void xs::inspector::render(double dt)
{
#ifdef PLATFORM_PS5
	return;
#endif

	// Use P to pause the game 
	if (input::get_key_once( input::KEY_P))
		game_paused = !game_paused;
	
	bool true_that = true;
	auto& io = ImGui::GetIO();	
	
	ImGui_Impl_NewFrame();
    ImGui::NewFrame();
	ok_timer -= dt;
	theme new_theme = current_theme;
	restart_flag = false;
	push_menu_theme();
	
    if (xs::script::has_error() ||
		show_registry			||
		show_profiler			||
		show_about				||
		show_demo				||
		show_modal				||
		show_inspector)
	{
        ImGui::Begin("Window", nullptr,
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetWindowPos({ 0, 0 });
		ImGui::SetWindowSize({-1, -1 });
    	
		if (game_paused)
		{
			if (ImGui::Button(ICON_FA_PLAY))
				game_paused = false;
			tooltip("Play");			
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_FAST_FORWARD))
				next_frame = true;
			tooltip("Next Frame");			
		}
		else
		{
			if (ImGui::Button(ICON_FA_PAUSE))
				game_paused = true;
			tooltip("Pause");
		}
    	
    	// Profiler
    	ImGui::SameLine();
    	if (ImGui::Button(ICON_FA_CHART_BAR))
    		show_profiler = !show_profiler;
    	tooltip("Profiler");
    	
		if (xs::get_run_mode() == run_mode::development)
		{
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_SYNC_ALT) || xs::input::get_key_once(xs::input::KEY_F5))
			{		
				script::shutdown();
				script::configure();
				script::initialize();
				if(!xs::script::has_error())
					ok_timer = 4.0f;
			}		
			tooltip("Reload Game");
    	
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_DATABASE))
				show_registry = !show_registry;
			tooltip("Data");
    	
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_IMAGES))
			{
				auto reloaded = render::reload_images();	
				next_frame = true;
				if (reloaded == 0)
					notify(notification_type::info, "No images reloaded", 3.0f);
				else
					notify(notification_type::success, to_string(reloaded) + " images reloaded", 3.0f);
			}
			tooltip("Reload Art");
		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_ADJUST))
		{
			int t = (int)current_theme;
			t = (t + 1) % 4;
			new_theme = (theme)t;
			data::set_number("theme", t, data::type::user);
			data::save_of_type(data::type::user);
		}
		tooltip("Theme");

#if SHOW_IMGUI_DEMO
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_LIFE_RING))
			show_demo = !show_demo;
		tooltip("Demo Window");
#endif

    	// About
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_QUESTION_CIRCLE))
			show_about = true;
		tooltip("About");
				
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
			tooltip("Script Error! Check output.");
			ImGui::PopStyleColor(1);
		}

		if (xs::data::has_chages()) {
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.745f, 0.000f, 0.949f, 1.000f));
			if (ImGui::Button(ICON_FA_EXCLAMATION_TRIANGLE)) {
				show_registry = true;
			}
			ImGui::PopStyleColor();
			tooltip("Data has unsaved changes");
		}
    	
    	// Close inspector
    	ImGui::SameLine();
    	if (ImGui::Button(ICON_FA_CARET_LEFT))
    	{
    		show_inspector = false;
    		data::set_bool("show_inspector", show_inspector, data::type::user);
    		data::save_of_type(data::type::user);
    	}
    	tooltip("Close Inspector");
    	
		ImGui::End();		
		
		{	// Bottom stats
			const auto& io = ImGui::GetIO();
			ImGui::Begin("Stats", &true_that,
						 ImGuiWindowFlags_NoScrollbar |
						 ImGuiWindowFlags_NoTitleBar |
						 ImGuiWindowFlags_NoResize |
						 ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoCollapse |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::SetWindowPos({ 0, io.DisplaySize.y - ImGui::GetWindowHeight() });
			ImGui::SetWindowSize({-1, -1 });
			ImGui::PushFont(small_font);
			
			auto mb = script::get_bytes_allocated() / (1024.0f * 1024.0f);
			auto mem_str = xs::tools::float_to_str_with_precision(mb, 1);
			auto stats = render::get_stats();
			auto draw_calls = to_string(stats.draw_calls);
			auto sprites = to_string(stats.sprites);
			auto textures =to_string(stats.textures);

        	
			auto path = fileio::absolute("[game]");
			ImGui::Text("%sMB | %sDC | %sSP | %s | %s",
						mem_str.c_str(),
						draw_calls.c_str(),
						sprites.c_str(),
						version::get_version_string(false, true, true).c_str(),
						path.c_str());
						
			ImGui::PopFont();
			ImGui::End();
		}
	}
	else
	{
		ImGui::Begin("Window", nullptr,
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetWindowPos({ 0, 0 });
		ImGui::SetWindowSize({-1, -1 });

		// A small button to show the inspector
		if (ImGui::Button(ICON_FA_CARET_SQUARE_RIGHT))
		{
			show_inspector = true;
			data::set_bool("show_inspector", show_inspector, data::type::user);
			data::save_of_type(data::type::user);
		}
		tooltip("Show Inspector");
		
		ImGui::End();		
	}
	
	{	// Notifications
		float x = -10.0f;
		float y = -10.0f;
		for (auto& n : notifications)
		{
			ImGui::Begin(to_string(n.id).c_str(), &true_that,
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
			ImGui::Text("%s", n.message.c_str());
			y -= ImGui::GetWindowHeight() + 10.0f;
			ImGui::End();
			n.time -= (float)dt;
		}

		notifications.erase(
			std::remove_if(
				notifications.begin(),
				notifications.end(),
				[](const notification& n) { return n.time <= 0.0f; }),
			notifications.end());
	}

	pop_menu_theme();	
		
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
		ImGui::Text(" xs %s ", xs::version::get_version_string().c_str());
		ImGui::Text(" Made with love at Breda University of Applied Sciences ");
		ImGui::End();
	}
	
	if(show_demo)
	{
		ImGui::ShowDemoWindow();
	}	
	
	ImGui::Render();    


	ImGui_Impl_RenderDrawData(ImGui::GetDrawData());	

	if (new_theme != current_theme)
	{
		current_theme = new_theme;
		apply_theme();
	}
}

bool xs::inspector::paused()
{
	bool t = game_paused && !next_frame;
	next_frame = false;
	return t;
}

bool xs::inspector::should_restart()
{
	return restart_flag;
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

void xs::inspector::apply_theme()
{
	switch (current_theme)
	{
	case theme::acrylic:
		see_through();
		break;
	case theme::dark:
		embrace_the_darkness();
		break;
	case theme::light:
		follow_the_light();
		break;
	case theme::gray:
		go_gray();
		break;
	}
	
	// Scale all style sizes by HDPI factor
	const float UIScale = (float)device::hdpi_scaling();
	if (UIScale != 1.0f)
		ImGui::GetStyle().ScaleAllSizes(UIScale);
}

void xs::inspector::push_menu_theme()
{
	switch (current_theme)
	{
	case theme::acrylic:
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.31f, 0.31f, 0.31f, 0.31f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.23f, 0.23f, 0.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		break;
	case theme::dark:
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);		
		break;
	case theme::light:
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		break;
	case theme::gray:
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		break;
	}
}

void xs::inspector::pop_menu_theme()
{
	switch (current_theme)
	{
	case theme::acrylic:
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(2);
		break;
	case theme::dark:
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(1);
		break;
	case theme::light:
		ImGui::PopStyleColor(1);
		break;
	case theme::gray:
		ImGui::PopStyleColor(1);
		break;
	}
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
	colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
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
	colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
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
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
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
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
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
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.50f, 0.50f, 0.50f, 0.5f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
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

void xs::inspector::see_through()
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 0.86f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.75f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.43f, 0.43f, 0.43f, 0.67f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.45f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.00f, 0.00f, 0.00f, 0.45f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.10f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.56f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.40f, 0.40f, 0.40f, 0.22f);
	colors[ImGuiCol_TabSelected]            = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_TabDimmed]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavCursor]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);



	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
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

#else

void xs::inspector::initialize() {}
void xs::inspector::shutdown() {}
void xs::inspector::render(double dt) {}
bool xs::inspector::paused() { return false; }
bool xs::inspector::should_restart() { return false; }
int xs::inspector::notify(notification_type type, const std::string& message, float time) { return 0; }
void xs::inspector::clear_notification(int id) {}

#endif
