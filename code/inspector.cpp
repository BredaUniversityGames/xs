#include "inspector.hpp"

#ifdef INSPECTOR
#include <imgui.h>
#include <imgui_impl.h>
#include <imgui_internal.h>
#include <implot.h>
#include "fluent_glyph.hpp"
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
#include "defines.hpp"

#if defined(EDITOR)
#include "dialogs/portable-file-dialogs.h"
#include <SDL3/SDL.h>
#endif

#if defined(PLATFORM_PC)
#include "device_pc.hpp"
#endif

#define SHOW_IMGUI_DEMO 1

// Inspector UI sizing constants
namespace
{
	// Frame dimensions
	#if defined(PLATFORM_APPLE)
		constexpr float c_frame_top_bar = 70.0f;
		constexpr float c_frame_bottom_bar = 55.0f;
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 16.0f;
		constexpr float c_small_font_size = 12.0f;
		constexpr float c_icon_font_size = 12.0f;
		constexpr float c_icon_vertical_offset = 0.0f;

	#elif defined(PLATFORM_PC)
		// Frame
		constexpr float c_frame_top_bar = 55.0f;
		constexpr float c_frame_bottom_bar = 45.0f;
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 16.0f;
		constexpr float c_small_font_size = 13.0f;
		constexpr float c_icon_font_size = 19.0f;
		constexpr float c_small_icon_font_size = 16.0f;
		constexpr float c_icon_vertical_offset = 5.0f;

	#else
		constexpr float c_frame_top_bar = 70.0f;
		constexpr float c_frame_bottom_bar = 55.0f;
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 16.0f;
		constexpr float c_small_font_size = 12.0f;
		constexpr float c_icon_font_size = 12.0f;
		constexpr float c_icon_vertical_offset = 0.0f;

	#endif

	// Game frame border
	constexpr float c_frame_rounding = 18.0f;
	constexpr float c_frame_thickness = c_frame_rounding;

	// Notifications
	constexpr float c_notification_offset_x = -10.0f;
	constexpr float c_notification_offset_y = -50.0f;
	constexpr float c_notification_spacing = 10.0f;

	// Timers
	constexpr float c_reload_ok_timer = 4.0f;
	constexpr float c_tooltip_hover_time = 0.6f;
	constexpr float c_notification_default_time = 3.0f;

	// Style scale multiplier
	#if defined(PLATFORM_APPLE)
		constexpr float c_style_scale = 1.0f;
	#elif defined(PLATFORM_PC)
		
	#else
		constexpr float c_style_scale = 1.0f;
	#endif

	// Fluent icon font filenames (tokens resolved via fileio::get_path)
	constexpr const char* kFluentIconFont = "[shared]/fonts/FluentSystemIcons-Regular.ttf";

	// Status bar alpha settings (adjustable)
	constexpr float c_status_button_base_alpha = 0.0f;
	constexpr float c_status_button_hover_alpha = 0.08f;
	constexpr float c_status_button_active_alpha = 0.12f;
	constexpr float c_status_text_alpha = 0.7f;
}

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
    bool always_on_top = false;
    char* ini_filename = nullptr;
    frame current_frame = { c_frame_top_bar, c_frame_bottom_bar };

    // color identifiers for inspector-specific palette
    enum class color_id { Green = 0, Blue, Orange, Purple, Pink, Gray, Red, Count };
    inline std::array<ImVec4, (size_t)color_id::Count> inspector_colors;

    inline ImVec4 get_color(color_id id)
    {
        return inspector_colors[(size_t)id];
    }

	// theme current_theme = theme::dark;
	void apply_theme(theme t);
	void push_menu_theme(theme t);
	void pop_menu_theme(theme t);
	void embrace_the_darkness();
	void follow_the_light();

	// Helper function to merge Fluent icons into a font
	ImFont* merge_fluent_icons(ImFont* base_font, float icon_size, float font_scale, const std::string& font_file = std::string());

	double ok_timer = 0.0f;
	bool next_frame;
	std::vector<notification> notifications;
	ImFont* small_font = nullptr;

	// Forward declarations for UI helpers
    static bool toggle_button(const char* icon, bool state, const char* tip);
    static bool colored_button(const char* icon, const ImVec4& color, const char* tip);
    static bool colored_button(const char* icon, color_id color_id, const char* tip);
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
    
#if defined(PLATFORM_APPLE)
	const float font_scale = 1.0f;
	ui_scale = 1.0f / (float)device::hdpi_scaling();
#elif defined(PLATFORM_PC)
	const float font_scale = (float)device::hdpi_scaling();
	ui_scale = 1.0f;
#endif
	const float font_size = c_font_size;
	const float icon_size = c_icon_font_size;
    
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
	auto selawk_font = io.Fonts->AddFontFromMemoryTTF(selawk_buffer, (int)selawk_data.size(), font_size * font_scale, &config);
	assert(selawk_font);

	// Merge Fluent icons into main font
	merge_fluent_icons(selawk_font, icon_size, font_scale, kFluentIconFont);

	// Copy to a heap allocation (again) to ensure it stays valid
	selawk_buffer = new char[selawk_data.size()];
	memcpy(selawk_buffer, selawk_data.data(), selawk_data.size());
	small_font = io.Fonts->AddFontFromMemoryTTF(selawk_buffer, (int)selawk_data.size(), c_small_font_size * font_scale, nullptr);
	assert(small_font);

	// Merge Fluent icons into small font
	merge_fluent_icons(small_font, c_small_icon_font_size, font_scale, kFluentIconFont);

	const std::string iniPath = fileio::get_path("[user]/imgui.ini");
	const char* constStr = iniPath.c_str();
	ini_filename = new char[iniPath.size() + 1];
	strcpy(ini_filename, constStr);
	io.IniFilename = ini_filename;
    
    auto current_theme = inspector::get_theme();
    apply_theme(current_theme);
	
	always_on_top = data::get_bool("always_on_top", data::type::user);
	if (always_on_top)
		device::toggle_on_top();
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
	if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > c_tooltip_hover_time)
	{
		ImGui::BeginTooltip();
		ImGui::SetTooltip("%s", tooltip);
		ImGui::EndTooltip();
	}	
}

void xs::inspector::render(double dt)
{
// TODO: Un-hack
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
    auto current_theme = get_theme();
	restart_flag = false;
	push_menu_theme(current_theme);
    float width = (float)device::get_width();
	
    {   // Game frame
        const float frame_rounding = c_frame_rounding;
        const float frame_thickness = c_frame_thickness;
        const float top_bar_height = inspector::current_frame.top_bar * ui_scale;
        const float bottom_bar_height = inspector::current_frame.bottom_bar * ui_scale;
        
        // Draw the rounded frame border behind all UI
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        
        const float game_height = io.DisplaySize.y - top_bar_height - bottom_bar_height;
        const float half_thickness = frame_thickness * 0.5f;
        
        // Frame rectangle centered on the border line
        ImVec2 frame_min = ImVec2(-half_thickness, top_bar_height - half_thickness);
        ImVec2 frame_max = ImVec2(width + half_thickness, top_bar_height + game_height + half_thickness);
        
        // Use the window background color from the current theme
        ImVec4 bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        ImU32 frame_color = ImGui::ColorConvertFloat4ToU32(bg_color);
        
        // Draw the rounded rect path with stroke
        draw_list->PathRect(frame_min, frame_max, frame_rounding);
        draw_list->PathStroke(frame_color, ImDrawFlags_Closed, frame_thickness);
    }
    
    {	// Top bar
        ImGui::Begin("Window", nullptr,
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetWindowPos({ 0, 0 });
        ImGui::SetWindowSize({width, inspector::current_frame.top_bar * ui_scale });
        
        // Playback controls: play/pause + next-frame (next-frame always visible, disabled when not paused)
        if (game_paused)
        {
            if (colored_button(ICON_FI_PLAY, get_color(color_id::Green), "Play"))
                game_paused = false;
        }
        else
        {
            if (colored_button(ICON_FI_PAUSE, get_color(color_id::Green), "Pause"))
                game_paused = true;
        }

        ImGui::SameLine();
        // "Next Frame" should always be visible but disabled when it can't be used
        ImGui::BeginDisabled(!game_paused);
        if (colored_button(ICON_FI_FAST_FORWARD, get_color(color_id::Green), "Next Frame"))
            next_frame = true;
        ImGui::EndDisabled();

        if (xs::get_run_mode() == run_mode::development)
        {
            ImGui::SameLine();
            if ((ImGui::Button(ICON_FI_SYNC_ALT) || xs::input::get_key_once(xs::input::KEY_F5)))
             {
                 script::shutdown();
                 script::configure();
                 script::initialize();
                 if(!xs::script::has_error())
                     ok_timer = c_reload_ok_timer;
             }
             tooltip("Reload game scripts (F5)");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FI_BARS))
                show_registry = !show_registry;
            tooltip("Open data registry");

            ImGui::SameLine();
            if (ImGui::Button(ICON_FI_IMAGE))
             {
                 auto reloaded = render::reload_images();
                 next_frame = true;
                 if (reloaded == 0)
                     notify(notification_type::info, "No images reloaded", c_notification_default_time);
                 else
                     notify(notification_type::success, to_string(reloaded) + " images reloaded", c_notification_default_time);
             }
            tooltip("Reload art assets");
        }

        // Profiler (use normal button)
        ImGui::SameLine();
        if (ImGui::Button(ICON_FI_PROFILER))
            show_profiler = !show_profiler;
        tooltip("Toggle profiler");
        
        ImGui::SameLine();
        // Simpler on-top toggle: show pin icon state and use a normal button for clarity
        {
            const char* pin_icon = always_on_top ? ICON_FI_PIN_ON : ICON_FI_PIN_OFF;
            if (ImGui::Button(pin_icon))
            {
                always_on_top = device::toggle_on_top();
                data::set_bool("always_on_top", always_on_top, data::type::user);
                data::save_of_type(data::type::user);
            }
            tooltip("Always on Top");
        }
        
#if SHOW_IMGUI_DEMO
        ImGui::SameLine();
        if (ImGui::Button(ICON_FI_WINDOW))
            show_demo = !show_demo;
        tooltip("Show ImGui demo window");
#endif


        // About
        ImGui::SameLine();
        if (colored_button(ICON_FI_QUESTION_CIRCLE, ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "About"))
            show_about = true;
        
        ImGui::SameLine();
        if (xs::script::has_error())
        {
            game_paused = true;
            if (ImGui::Button(ICON_FI_TIMES_CIRCLE))
            {
                xs::script::clear_error();
                game_paused = false;
            }
            tooltip("Script Error! Check output.");
        }

        if (xs::data::has_chages())
        {
            ImGui::SameLine();
            if (ImGui::Button(ICON_FI_EXCLAMATION_TRIANGLE)) {
                show_registry = true;
            }
            tooltip("Data has unsaved changes");
        }
        
        ImGui::End();
    }
        
    {    // Bottom stats
        const auto& io = ImGui::GetIO();
        ImGui::Begin("Stats", &true_that,
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetWindowPos({ 0, io.DisplaySize.y - current_frame.bottom_bar * ui_scale });
        ImGui::SetWindowSize({width, current_frame.bottom_bar * ui_scale });
        ImGui::PushFont(small_font);

        auto mb = script::get_bytes_allocated() / (1024.0f * 1024.0f);
        auto mem_str = xs::tools::float_to_str_with_precision(mb, 1);
        auto stats = render::get_stats();
        auto draw_calls = to_string(stats.draw_calls);
        auto sprites = to_string(stats.sprites);
        auto textures = to_string(stats.textures);
        auto path = fileio::absolute("[game]");

        // Use transparent button style so they look like status items. Alpha is configurable.
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,c_status_button_base_alpha));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,c_status_button_hover_alpha));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,c_status_button_active_alpha));
        // Push text color with adjustable alpha for labels in the status bar
        ImVec4 base_text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(base_text.x, base_text.y, base_text.z, c_status_text_alpha));

        // Memory button (icon + text). Stable ID after '##' keeps identity while text changes.
        {
            std::string label = std::string(ICON_FI_MEMORY) + " " + mem_str + "MB" + "##stat_mem";
            if (ImGui::Button(label.c_str())) {
                 notify(notification_type::info, std::string("Memory: ") + mem_str + " MB", c_notification_default_time);
             }
             tooltip("Memory allocated (MB)");
        }

        ImGui::SameLine();

        // Draw calls
        {
            std::string label = std::string(ICON_FI_IMAGE_PEN) + " " + draw_calls + "##stat_dc";
            if (ImGui::Button(label.c_str())) {
                 // Toggle profiler on click
                 show_profiler = !show_profiler;
             }
             tooltip("Draw calls this frame (click to toggle profiler)");
        }

        ImGui::SameLine();

        // Sprites
        {
            std::string label = std::string(ICON_FI_IMAGE_FRAME) + " " + sprites + "##stat_sprites";
            if (ImGui::Button(label.c_str())) {
                 notify(notification_type::info, std::string("Sprites: ") + sprites, c_notification_default_time);
             }
             tooltip("Sprites drawn this frame");
        }

        ImGui::SameLine();

        // Textures
        {
            std::string label = std::string(ICON_FI_IMAGES) + " " + textures + "##stat_textures";
            if (ImGui::Button(label.c_str())) {
                 notify(notification_type::info, std::string("Textures: ") + textures, c_notification_default_time);
             }
             tooltip("Number of textures loaded in GPU memory");
        }

        ImGui::SameLine();

        // Version
        {
            auto ver = version::get_version_string(false, true, true);
            std::string label = std::string(ICON_FI_TAG) + " " + ver + "##stat_version";
            if (ImGui::Button(label.c_str())) {
                ImGui::SetClipboardText(ver.c_str());
                notify(notification_type::info, "Version copied to clipboard", c_notification_default_time);
            }
            tooltip("xs version (click to copy)");
        }

        ImGui::SameLine();

        // Path (clickable) - use remaining width so full path is visible
        {
            // ensure we're on the same line with previous separator already placed
            std::string full_path = path;
            std::string label = std::string(ICON_FI_FOLDER) + " " + full_path + "##stat_path";

            // Make selectable span the remaining width and appear unstyled
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0));

            float avail = ImGui::GetContentRegionAvail().x;
            if (avail < 1.0f) avail = 1.0f;
            // Use the current frame height so the selectable isn't clipped vertically
            float height = ImGui::GetFrameHeight();
            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth, ImVec2(avail, height))) {
                ImGui::SetClipboardText(full_path.c_str());
                notify(notification_type::info, "Path copied to clipboard", c_notification_default_time);
            }
            tooltip("Click to copy the game folder path to clipboard");

            ImGui::PopStyleColor(3);
        }

        // Pop text color and button colors
        ImGui::PopStyleColor(4);
         ImGui::PopFont();
         ImGui::End();
 	}
	
	{	// Notifications
		float x = c_notification_offset_x;
		float y = c_notification_offset_y;
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
				color = get_color(color_id::Blue);
				icon = ICON_FI_INFO_CIRCLE;
				break;
			case notification_type::success:
				color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				icon = ICON_FI_CHECK_CIRCLE;
				break;
			case notification_type::warning:
				color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
				icon = ICON_FI_EXCLAMATION_TRIANGLE;
				break;
			case notification_type::error:
			 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
				icon = ICON_FI_TIMES_CIRCLE;
				break;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Button(icon.c_str());			
			ImGui::PopStyleColor(1);
			ImGui::SameLine();
			ImGui::Text("%s", n.message.c_str());
			y -= ImGui::GetWindowHeight() + c_notification_spacing;
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

	pop_menu_theme(current_theme);
		
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

    /*
	if (new_theme != current_theme)
	{
		current_theme = new_theme;
		apply_theme();
	}
    */
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

theme xs::inspector::get_theme()
{
#if defined(EDITOR)
    auto sdl_theme = SDL_GetSystemTheme();
    switch (sdl_theme) {
        case SDL_SYSTEM_THEME_LIGHT: return theme::light;
        case SDL_SYSTEM_THEME_DARK: return theme::dark;
        case SDL_SYSTEM_THEME_UNKNOWN: return theme::dark;
    }
#endif
    return theme::dark;
}

void xs::inspector::apply_theme(theme t)
{
    switch (t)
    {
    case theme::dark:
        embrace_the_darkness();
        break;
    case theme::light:
        follow_the_light();
        break;
    }

#if defined(PLATFORM_PC)
    auto& style = ImGui::GetStyle();
    float scale = (float)device::hdpi_scaling();
    style.ScaleAllSizes(scale);
#endif
}

void xs::inspector::push_menu_theme(theme t)
{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0f, 0.0f));
}

void xs::inspector::pop_menu_theme(theme t)
{
	ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(4);
}

frame xs::inspector::get_frame()
{
    return inspector::current_frame;
}

static bool xs::inspector::colored_button(const char* icon, const ImVec4& color, const char* tip)
{
    // Color the text/icon instead of the button background
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    bool clicked = ImGui::Button(icon);
    ImGui::PopStyleColor(1);

    if (tip)
        tooltip(tip);

    return clicked;
}

// Implement overload that uses inspector_color_id
static bool xs::inspector::colored_button(const char* icon, color_id color_id, const char* tip)
{
    ImVec4 c = get_color(color_id);
    return colored_button(icon, c, tip);
}

// Implement toggle_button
static bool xs::inspector::toggle_button(const char* icon, bool state, const char* tip)
{
    ImGui::SameLine();

    // base (off) colors from the current theme
    ImVec4 normal       = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    ImVec4 normalHover  = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    ImVec4 normalActive = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);

    // derive "on" (accent) colors from theme: prefer HeaderActive/Hovered, fallback to ButtonActive variants
    ImVec4 accent       = ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive);
    ImVec4 accentHover  = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);
    ImVec4 accentActive = ImGui::GetStyleColorVec4(ImGuiCol_Header);

    // if header colors are basically default/transparent in some themes, fallback to ButtonActive variants
    auto is_transparent = [](const ImVec4 &c){ return c.x==0.0f && c.y==0.0f && c.z==0.0f && c.w==0.0f; };
    if (is_transparent(accent) && !is_transparent(normalActive)) {
        accent       = normalActive;
        accentHover  = normalHover;
        accentActive = normalActive;
    }

    ImGui::PushStyleColor(ImGuiCol_Button,        state ? accent    : normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, state ? accentHover: normalHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  state ? accentActive: normalActive);

    bool clicked = ImGui::Button(icon);

    ImGui::PopStyleColor(3);

    if (tip)
        tooltip(tip);

    return clicked;
}

void apply_common_style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// Padding and spacing
	style.WindowPadding = ImVec2(8.00f * c_style_scale, 8.00f * c_style_scale);
	style.FramePadding = ImVec2(5.00f * c_style_scale, 2.00f * c_style_scale);
	style.ItemSpacing = ImVec2(6.00f * c_style_scale, 6.00f * c_style_scale);
	style.ItemInnerSpacing = ImVec2(6.00f * c_style_scale, 6.00f * c_style_scale);
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);

	// Sizes
	style.IndentSpacing = 25 * c_style_scale;
	style.ScrollbarSize = 15 * c_style_scale;
	style.GrabMinSize = 10 * c_style_scale;

	// Borders
	style.WindowBorderSize = 1 * c_style_scale;
	style.ChildBorderSize = 1 * c_style_scale;
	style.PopupBorderSize = 1 * c_style_scale;
	style.FrameBorderSize = 1 * c_style_scale;
	style.TabBorderSize = 1 * c_style_scale;

	// Rounding
	style.WindowRounding = 7 * c_style_scale;
	style.ChildRounding = 4 * c_style_scale;
	style.FrameRounding = 3 * c_style_scale;
	style.PopupRounding = 4 * c_style_scale;
	style.ScrollbarRounding = 9 * c_style_scale;
	style.GrabRounding = 3 * c_style_scale;
	style.TabRounding = 4 * c_style_scale;

	// Other
	style.LogSliderDeadzone = 4 * c_style_scale;
}

void xs::inspector::embrace_the_darkness()
{
	// Try to get system colors for titlebar and window background
	ImVec4 windowBg = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	ImVec4 titleBg = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	ImVec4 titleBgActive = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	
#if defined(PLATFORM_PC)
	// Windows dark theme: slightly lighter dark gray
	windowBg = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	titleBg = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	titleBgActive = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
#elif defined(PLATFORM_APPLE)
	// macOS dark theme: deeper blacks with subtle warmth
    windowBg = ImColor(35, 34, 48, 255);
    titleBg = windowBg;
	titleBgActive = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
#endif

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = windowBg;
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
	colors[ImGuiCol_TitleBg] = titleBg;
	colors[ImGuiCol_TitleBgActive] = titleBgActive;
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

	apply_common_style();

	// Populate inspector-specific palette for dark theme
    inspector_colors[(size_t)color_id::Green] = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    inspector_colors[(size_t)color_id::Blue] = ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
    inspector_colors[(size_t)color_id::Orange] = ImVec4(0.9f, 0.6f, 0.2f, 1.0f);
    inspector_colors[(size_t)color_id::Purple] = ImVec4(0.5f, 0.4f, 0.8f, 1.0f);
    inspector_colors[(size_t)color_id::Pink] = ImVec4(0.8f, 0.3f, 0.7f, 1.0f);
    inspector_colors[(size_t)color_id::Gray] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    inspector_colors[(size_t)color_id::Red] = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
}

void xs::inspector::follow_the_light()
{
	// Try to get system colors for titlebar and window backgrounds
	ImVec4 windowBg = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	ImVec4 titleBg = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	ImVec4 titleBgActive = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	
#if defined(PLATFORM_PC)
	// Windows light theme: lighter gray for inactive, slightly darker for active
	windowBg = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
	titleBg = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
	titleBgActive = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
#elif defined(PLATFORM_APPLE)
	// macOS light theme: very light gray, almost white
	windowBg = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
	titleBg = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
	titleBgActive = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
#endif

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_WindowBg] = windowBg;
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg] = titleBg;
	colors[ImGuiCol_TitleBgActive] = titleBgActive;
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

	apply_common_style();

	// Populate inspector-specific palette for light theme
    inspector_colors[(size_t)color_id::Green] = ImVec4(0.07f, 0.6f, 0.07f, 1.0f);
    inspector_colors[(size_t)color_id::Blue] = ImVec4(0.07f, 0.45f, 0.8f, 1.0f);
    inspector_colors[(size_t)color_id::Orange] = ImVec4(0.8f, 0.5f, 0.08f, 1.0f);
    inspector_colors[(size_t)color_id::Purple] = ImVec4(0.45f, 0.35f, 0.75f, 1.0f);
    inspector_colors[(size_t)color_id::Pink] = ImVec4(0.75f, 0.28f, 0.65f, 1.0f);
    inspector_colors[(size_t)color_id::Gray] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    inspector_colors[(size_t)color_id::Red] = ImVec4(0.85f, 0.15f, 0.15f, 1.0f);
}

// Add missing helper implementations
ImFont* xs::inspector::merge_fluent_icons(ImFont* base_font, float icon_size, float font_scale, const std::string& font_file)
{
    auto& io = ImGui::GetIO();
    const ImWchar* fluent_icons_ranges = xs::tools::get_fluent_glyph_ranges();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphOffset = ImVec2(0.0f, c_icon_vertical_offset * font_scale);
    config.OversampleH = 8;
    config.OversampleV = 8;

    std::string token = font_file.empty() ? std::string(kFluentIconFont) : font_file;
    std::string fluent_font = fileio::get_path(token);
    if(!fileio::exists(fluent_font))
    {
        log::critical("Could not find the fluent icon font at path:{}", fluent_font);
        return base_font;
    }

    auto fluent_font_data = fileio::read_binary_file(fluent_font);
    char* fluent_font_buffer = new char[fluent_font_data.size()];
    memcpy(fluent_font_buffer, fluent_font_data.data(), fluent_font_data.size());

    auto fluent_icons_font = io.Fonts->AddFontFromMemoryTTF(
        fluent_font_buffer,
        (int)fluent_font_data.size(),
        icon_size * font_scale,
        &config,
        fluent_icons_ranges);
    assert(fluent_icons_font != nullptr);

    return base_font;
}

#endif
