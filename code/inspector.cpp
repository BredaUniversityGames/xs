#include "inspector.hpp"

#ifdef INSPECTOR
#include <imgui.h>
#include "imgui_impl.h"
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
#include "sdl3/device_sdl.hpp"
#endif

#define SHOW_IMGUI_DEMO 1

// Inspector UI sizing constants
namespace
{
	// Frame dimensions
	#if defined(PLATFORM_APPLE)
		// Frame
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 16.0f;
		constexpr float c_small_font_size = 12.0f;
		constexpr float c_icon_font_size = 16.0f;
		constexpr float c_small_icon_font_size = 14.0f;
		constexpr float c_icon_vertical_offset = 4.0f;

	#elif defined(PLATFORM_PC)
		// Frame
		constexpr float c_frame_top_bar = 40.0f;
        constexpr float c_frame_bottom_bar = 40.0f;
        constexpr float c_right_panel_width = 450.0f;
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 16.0f;
		constexpr float c_small_font_size = 13.0f;
		constexpr float c_icon_font_size = 19.0f;
		constexpr float c_small_icon_font_size = 16.0f;
		constexpr float c_icon_vertical_offset = 4.8f;

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
	constexpr float c_notification_offset_y = -10.0f;
	constexpr float c_notification_spacing = 10.0f;

	// Timers
	constexpr float c_reload_ok_timer = 4.0f;
	constexpr float c_tooltip_hover_time = 0.6f;
	constexpr float c_notification_default_time = 3.0f;

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

    enum class right_panel_mode { none, data, profiler };

    enum class zoom_mode { fit, zoom_50, zoom_100, zoom_150, zoom_200 };

    bool game_paused = false;
    bool restart_flag = false;
    float ui_scale = 1.0f;
    right_panel_mode right_panel = right_panel_mode::none;
    zoom_mode current_zoom = zoom_mode::fit;
    bool show_about = false;
    bool show_demo = false;
    bool show_modal = false;
    bool show_inspector = false;
    bool always_on_top = false;
    char* ini_filename = nullptr;
    ImGuiID dock_id_top = {};
    ImGuiID dock_id_bottom = {};

    // Game viewport bounds (screen space)
    ImVec2 game_viewport_min = {};
    ImVec2 game_viewport_max = {};
    ImVec2 game_viewport_size = {};

    // color identifiers for inspector-specific palette
    enum class color_id { Green = 0, Blue, Orange, Purple, Pink, Gray, Red, Count };
    inline std::array<ImVec4, (size_t)color_id::Count> inspector_colors;

    inline ImVec4 get_color(color_id id)
    {
        return inspector_colors[(size_t)id];
    }

    enum class theme { light, dark };
    theme get_theme();
    theme current_theme = theme::dark;
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
    static void vertical_separator(float alpha = 0.15f, float height = 0.0f);

    // Forward declarations for render sections
    static void render_top_bar();
    static void render_stats_bar();
    static void render_game_viewport();
    static void render_notifications(double dt);
    static void render_right_panel();
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

	// Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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
	// Explicitly save ImGui settings before shutdown
	ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);

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
	
	ImGui_Impl_NewFrame();
    ImGui::NewFrame();
	ok_timer -= dt;
    auto current_theme = get_theme();
	restart_flag = false;
	push_menu_theme(current_theme);

	// Create fullscreen dockspace
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
	                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
	                                   ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", nullptr, dockspace_flags);
	ImGui::PopStyleVar();

	ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// Set up default docking layout on first run
	static bool first_time = true;
	if (first_time)
	{
		first_time = false;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);
		
		// Split the dockspace into sections using calculated ratios
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.4f, nullptr, &dockspace_id);
        dock_id_top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.05f, nullptr, &dockspace_id);
		dock_id_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.01f, nullptr, &dockspace_id);
		
		// Configure all docks: no tab bars
        ImGuiDockNode* node_right = ImGui::DockBuilderGetNode(dock_id_right);
		ImGuiDockNode* node_top = ImGui::DockBuilderGetNode(dock_id_top);
		ImGuiDockNode* node_bottom = ImGui::DockBuilderGetNode(dock_id_bottom);
		ImGuiDockNode* node_center = ImGui::DockBuilderGetNode(dockspace_id);
		node_top->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
		node_bottom->LocalFlags |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoResize;
		node_center->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
		node_right->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

		// Dock windows into their positions
		ImGui::DockBuilderDockWindow("Top Bar", dock_id_top);
		ImGui::DockBuilderDockWindow("Stats", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Data Registry", dock_id_right);
		ImGui::DockBuilderDockWindow("Game", dockspace_id);  // Game viewport in center
		ImGui::DockBuilderDockWindow("Profiler", dock_id_right);

		ImGui::DockBuilderFinish(dockspace_id);
	}

	pop_menu_theme(current_theme);

    render_top_bar();
	render_stats_bar();
	render_game_viewport();
	render_notifications(dt);
	render_right_panel();

	if(show_demo)
	{
		ImGui::ShowDemoWindow();
	}

	// End dockspace
	ImGui::End();

	ImGui::Render();
	ImGui_Impl_RenderDrawData(ImGui::GetDrawData());
}

// Render section implementations

static void xs::inspector::render_top_bar()
{
	auto current_theme = get_theme();
	push_menu_theme(current_theme);

	// Measure desired height (one row example)
	float desired_h = ImGui::GetFrameHeightWithSpacing(); // convenience: button/input height + spacing
	desired_h += ImGui::GetStyle().WindowPadding.y * 2.0f; // extra padding
	ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock_id_top);
	n->SizeRef.y = desired_h;     // height you want (in pixels)
	n->Size.y    = desired_h;     // optional, for immediate effect

	ImGui::Begin("Top Bar", nullptr,
				 ImGuiWindowFlags_NoScrollbar |
				 ImGuiWindowFlags_NoTitleBar |
				 ImGuiWindowFlags_NoCollapse |
				 ImGuiWindowFlags_NoScrollWithMouse |
				 ImGuiWindowFlags_NoResize |
				 ImGuiWindowFlags_NoMove);

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
        
        // Soft vertical separator
        vertical_separator();

        if (xs::get_run_mode() == run_mode::development)
        {
            ImGui::SameLine();
            if (colored_button(ICON_FI_SYNC_ALT, get_color(color_id::Orange), "Reload game scripts (F5)") || xs::input::get_key_once(xs::input::KEY_F5))
            {
                 script::shutdown();
                 script::configure();
                 script::initialize();
                 if(!xs::script::has_error())
                     ok_timer = c_reload_ok_timer;
             }

            ImGui::SameLine();
            if (colored_button(ICON_FI_IMAGE, get_color(color_id::Orange), "Reload art assets"))
            {
                 auto reloaded = render::reload_images();
                 next_frame = true;
                 if (reloaded == 0)
                     notify(notification_type::info, "No images reloaded", c_notification_default_time);
                 else
                     notify(notification_type::success, to_string(reloaded) + " images reloaded", c_notification_default_time);
            }
        
            vertical_separator();

            // Data Registry toggle button
            if (toggle_button(ICON_FI_BARS, right_panel == right_panel_mode::data, "Data registry"))
            {
                right_panel = (right_panel == right_panel_mode::data) ? right_panel_mode::none : right_panel_mode::data;
            }

            // Profiler toggle button
            if (toggle_button(ICON_FI_PROFILER, right_panel == right_panel_mode::profiler, "Profiler"))
            {
                right_panel = (right_panel == right_panel_mode::profiler) ? right_panel_mode::none : right_panel_mode::profiler;
            }
        }

        // Always on top toggle button
        if (toggle_button(always_on_top ? ICON_FI_PIN_ON : ICON_FI_PIN_OFF, always_on_top, "Always on Top"))
        {
            always_on_top = device::toggle_on_top();
            data::set_bool("always_on_top", always_on_top, data::type::user);
            data::save_of_type(data::type::user);
        }

        // Zoom dropdown
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        const char* zoom_labels[] = { "Fit", "50%", "100%", "150%", "200%" };
        int current_zoom_idx = (int)current_zoom;
        if (ImGui::Combo("##zoom", &current_zoom_idx, zoom_labels, IM_ARRAYSIZE(zoom_labels)))
        {
            current_zoom = (zoom_mode)current_zoom_idx;
        }
        tooltip("Game viewport zoom");

#if SHOW_IMGUI_DEMO
        if (toggle_button(ICON_FI_WINDOW, show_demo, "Show ImGui demo window"))
            show_demo = !show_demo;
#endif

        vertical_separator();

		ImGui::SameLine();
        if (xs::script::has_error())
        {
            game_paused = true;
            if (colored_button(ICON_FI_TIMES_CIRCLE, get_color(color_id::Red), "Script Error! Check output."))
            {
                xs::script::clear_error();
                game_paused = false;
            }
        }

	ImGui::SameLine();
	if (xs::data::has_chages())
	{
		if (colored_button(ICON_FI_EXCLAMATION_TRIANGLE, get_color(color_id::Purple), "Data has unsaved changes")) {
			right_panel = right_panel_mode::data;
		}
	}

	ImGui::End();
	pop_menu_theme(current_theme);
}

static void xs::inspector::render_stats_bar()
{
	ImGui::PushFont(small_font);

	// Measure desired height (one row example)
	float desired_h = ImGui::GetFrameHeightWithSpacing(); // convenience: button/input height + spacing
	desired_h += ImGui::GetStyle().WindowPadding.y * 2.0f; // extra padding
	ImGuiDockNode* n = ImGui::DockBuilderGetNode(dock_id_bottom);
	n->SizeRef.y = desired_h;     // height you want (in pixels)
	n->Size.y    = desired_h;     // optional, for immediate effect

	ImGui::Begin("Stats", nullptr,
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoResize);
        

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
			ImGui::Button(label.c_str());
            tooltip("Draw calls this frame (click to toggle profiler)");
        }

        ImGui::SameLine();

        // Sprites
        {
            std::string label = std::string(ICON_FI_IMAGE_FRAME) + " " + sprites + "##stat_sprites";
            ImGui::Button(label.c_str());
            tooltip("Sprites drawn this frame");
        }

        ImGui::SameLine();

        // Textures
        {
            std::string label = std::string(ICON_FI_IMAGES) + " " + textures + "##stat_textures";
            ImGui::Button(label.c_str());
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
	ImGui::End();
	ImGui::PopFont();
}

static void xs::inspector::render_game_viewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		// Enable scrollbars for fixed zoom modes
		ImGuiWindowFlags game_window_flags = ImGuiWindowFlags_NoCollapse |
		                                     ImGuiWindowFlags_NoTitleBar |
		                                     ImGuiWindowFlags_NoMove;
		if (current_zoom != zoom_mode::fit)
		{
			game_window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
		}
		ImGui::Begin("Game", nullptr, game_window_flags);

		// Get the game render target texture
		auto texture = xs::render::get_render_target_texture();

		// Get available content region (respects window padding/borders)
		ImVec2 available_region = ImGui::GetContentRegionAvail();

		// Calculate aspect ratio
		float game_width = (float)xs::configuration::width();
		float game_height = (float)xs::configuration::height();
		float aspect_ratio = game_width / game_height;

		// Calculate size based on zoom mode
		ImVec2 image_size;
		if (current_zoom == zoom_mode::fit)
		{
			// Best fit - maintain aspect ratio
			image_size = available_region;
			if (available_region.x / available_region.y > aspect_ratio)
			{
				// Window is wider than game - fit to height
				image_size.x = available_region.y * aspect_ratio;
			}
			else
			{
				// Window is taller than game - fit to width
				image_size.y = available_region.x / aspect_ratio;
			}
		}
		else
		{
			// Fixed zoom percentages
			float zoom_factor = 1.0f;
			switch (current_zoom)
			{
				case zoom_mode::zoom_50:  zoom_factor = 0.5f;  break;
				case zoom_mode::zoom_100: zoom_factor = 1.0f;  break;
				case zoom_mode::zoom_150: zoom_factor = 1.5f;  break;
				case zoom_mode::zoom_200: zoom_factor = 2.0f;  break;
				default: zoom_factor = 1.0f; break;
			}
			image_size.x = game_width * zoom_factor;
			image_size.y = game_height * zoom_factor;
		}

		// Round image size to whole numbers to avoid sampling artifacts
		image_size.x = floorf(image_size.x);
		image_size.y = floorf(image_size.y);

		// Center the image in the available space (only if it fits)
		ImVec2 cursor_pos = ImGui::GetCursorPos();
		if (image_size.x < available_region.x)
			cursor_pos.x += floorf((available_region.x - image_size.x) * 0.5f);
		if (image_size.y < available_region.y)
			cursor_pos.y += floorf((available_region.y - image_size.y) * 0.5f);
		ImGui::SetCursorPos(cursor_pos);

		// Get screen position for drawing and round to whole numbers
		ImVec2 screen_pos = ImGui::GetCursorScreenPos();
		screen_pos.x = floorf(screen_pos.x);
		screen_pos.y = floorf(screen_pos.y);
		ImVec2 screen_pos_max = ImVec2(screen_pos.x + image_size.x, screen_pos.y + image_size.y);

		// Store viewport bounds for mouse position API
		game_viewport_min = screen_pos;
		game_viewport_max = screen_pos_max;
		game_viewport_size = image_size;

		// Draw image using DrawList
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

#if defined(PLATFORM_APPLE)
		draw_list->AddImage((ImTextureID)texture, screen_pos, screen_pos_max,
		                    ImVec2(0, 0), ImVec2(1, 1),
		                    IM_COL32_WHITE);
#elif defined(PLATFORM_PC)
		draw_list->AddImage((ImTextureID)texture, screen_pos, screen_pos_max,
		                    ImVec2(0, 1), ImVec2(1, 0),
		                    IM_COL32_WHITE);
#endif

		// Draw corner masks on top to create rounded corners
		float rounding = 8.0f;
		ImVec4 bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		ImU32 bg_col32 = ImGui::ColorConvertFloat4ToU32(bg_color);

		// For each corner, draw a path that covers the corner area but leaves the rounded part
		// Extend 1 pixel outward to ensure clean edges
		// Top-left corner
		draw_list->PathLineTo(ImVec2(screen_pos.x - 1, screen_pos.y - 1));
		draw_list->PathLineTo(ImVec2(screen_pos.x + rounding, screen_pos.y - 1));
		draw_list->PathArcTo(ImVec2(screen_pos.x + rounding, screen_pos.y + rounding), rounding, -IM_PI * 0.5f, -IM_PI, 12);
		draw_list->PathLineTo(ImVec2(screen_pos.x - 1, screen_pos.y + rounding));
		draw_list->PathFillConvex(bg_col32);

		// Top-right corner
		draw_list->PathLineTo(ImVec2(screen_pos_max.x + 1, screen_pos.y - 1));
		draw_list->PathLineTo(ImVec2(screen_pos_max.x + 1, screen_pos.y + rounding));
		draw_list->PathArcTo(ImVec2(screen_pos_max.x - rounding, screen_pos.y + rounding), rounding, 0.0f, -IM_PI * 0.5f, 12);
		draw_list->PathLineTo(ImVec2(screen_pos_max.x - rounding, screen_pos.y - 1));
		draw_list->PathFillConvex(bg_col32);

		// Bottom-left corner
		draw_list->PathLineTo(ImVec2(screen_pos.x - 1, screen_pos_max.y + 1));
		draw_list->PathLineTo(ImVec2(screen_pos.x - 1, screen_pos_max.y - rounding));
		draw_list->PathArcTo(ImVec2(screen_pos.x + rounding, screen_pos_max.y - rounding), rounding, -IM_PI, -IM_PI * 1.5f, 12);
		draw_list->PathLineTo(ImVec2(screen_pos.x + rounding, screen_pos_max.y + 1));
		draw_list->PathFillConvex(bg_col32);

		// Bottom-right corner
		draw_list->PathLineTo(ImVec2(screen_pos_max.x + 1, screen_pos_max.y + 1));
		draw_list->PathLineTo(ImVec2(screen_pos_max.x - rounding, screen_pos_max.y + 1));
		draw_list->PathArcTo(ImVec2(screen_pos_max.x - rounding, screen_pos_max.y - rounding), rounding, IM_PI * 0.5f, 0.0f, 12);
		draw_list->PathLineTo(ImVec2(screen_pos_max.x + 1, screen_pos_max.y - rounding));
		draw_list->PathFillConvex(bg_col32);

	// Dummy to reserve space
	ImGui::Dummy(image_size);
	ImGui::End();
	ImGui::PopStyleVar();
}

static void xs::inspector::render_notifications(double dt)
{
	float x = c_notification_offset_x;
	float y = c_notification_offset_y;
	for (auto& n : notifications)
		{
			ImGui::SetNextWindowPos(ImVec2(game_viewport_max.x + x, game_viewport_max.y + y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
			ImGui::Begin(to_string(n.id).c_str(), nullptr,
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoInputs);

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
				color = get_color(color_id::Green);
				icon = ICON_FI_CHECK_CIRCLE;
				break;
			case notification_type::warning:
				color = get_color(color_id::Orange);
				icon = ICON_FI_EXCLAMATION_TRIANGLE;
				break;
			case notification_type::error:
				color = get_color(color_id::Red);
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

static void xs::inspector::render_right_panel()
{
	if (right_panel == right_panel_mode::data)
	{
		bool open = true;
		xs::data::inspect_at(open, 0, 0, 0, 0);
	}

	if (right_panel == right_panel_mode::profiler)
	{
		bool open = true;
		xs::profiler::inspect(open);
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

float xs::inspector::get_game_mouse_x()
{
	ImVec2 mouse_pos = ImGui::GetMousePos();

	// Convert to viewport-relative position (0 to viewport_size.x)
	float local_x = mouse_pos.x - game_viewport_min.x;

	// Convert to normalized coordinates (0 to 1)
	float normalized_x = local_x / game_viewport_size.x;

	// Convert to game coordinates centered at origin (-width/2 to width/2)
	float game_width = (float)xs::configuration::width();
	float x = (normalized_x * game_width) - (game_width * 0.5f);

	// Clamp to game bounds
	float half_width = game_width * 0.5f;
	if (x < -half_width) x = -half_width;
	if (x > half_width) x = half_width;

	return x;
}

float xs::inspector::get_game_mouse_y()
{
	ImVec2 mouse_pos = ImGui::GetMousePos();

	// Convert to viewport-relative position (0 to viewport_size.y)
	float local_y = mouse_pos.y - game_viewport_min.y;

	// Convert to normalized coordinates (0 to 1)
	float normalized_y = local_y / game_viewport_size.y;

	// Flip Y axis (ImGui has Y down, game has Y up) and convert to game coordinates centered at origin
	float game_height = (float)xs::configuration::height();
	float y = ((1.0f - normalized_y) * game_height) - (game_height * 0.5f);

	// Clamp to game bounds
	float half_height = game_height * 0.5f;
	if (y < -half_height) y = -half_height;
	if (y > half_height) y = half_height;

	return y;
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
        case SDL_SYSTEM_THEME_UNKNOWN: return theme::light;
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
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 0.0f));
}

void xs::inspector::pop_menu_theme(theme t)
{
    ImGui::PopStyleVar(3);
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

    // Use ButtonActive color for "on" state
    ImVec4 accent       = normalActive;
    ImVec4 accentActive = normalActive;

    ImGui::PushStyleColor(ImGuiCol_Button,        state ? accent    : normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  state ? accentActive: normalActive);

    bool clicked = ImGui::Button(icon);

    ImGui::PopStyleColor(2);

    if (tip)
        tooltip(tip);

    return clicked;
}

static void xs::inspector::vertical_separator(float alpha, float height)
{
    ImGui::SameLine();

    // If height is specified, use a custom dummy + line; otherwise use default separator
    if (height > 0.0f)
    {
        ImVec4 text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImVec4 sep_color = ImVec4(text_color.x, text_color.y, text_color.z, alpha);

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        ImGui::Dummy(ImVec2(spacing * 0.5f, height));
        ImGui::SameLine();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddLine(ImVec2(pos.x, pos.y), ImVec2(pos.x, pos.y + height), ImGui::ColorConvertFloat4ToU32(sep_color), 1.0f);

        ImGui::Dummy(ImVec2(spacing * 0.5f, height));
    }
    else
    {
        ImVec4 text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(text_color.x, text_color.y, text_color.z, alpha));
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::PopStyleColor();
    }
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
	style.WindowBorderSize = 0 * c_style_scale;
	style.ChildBorderSize = 0 * c_style_scale;
	style.PopupBorderSize = 1 * c_style_scale;
	style.FrameBorderSize = 0 * c_style_scale;
	style.TabBorderSize = 0 * c_style_scale;
	style.TabBarBorderSize = 0 * c_style_scale;

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
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.14f, 0.13f, 0.19f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.23f, 0.22f, 0.31f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(1.00f, 1.00f, 1.00f, 0.29f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.14f, 0.13f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(1.00f, 1.00f, 1.00f, 0.29f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.50f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.50f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_InputTextCursor]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TabSelected]            = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
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
    colors[ImGuiCol_TreeLines]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DragDropTargetBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_NavCursor]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
	colors[ImGuiCol_WindowBg] = windowBg;
	colors[ImGuiCol_TitleBg] = titleBg;
	colors[ImGuiCol_TitleBgActive] = titleBgActive;
	
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
    colors[ImGuiCol_WindowBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.70f, 0.70f, 0.7f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.08f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.18f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.16f);
    colors[ImGuiCol_Tab] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.00f, 0.00f, 0.00f, 0.08f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
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

#else

// Stub implementations for non-EDITOR builds
void xs::inspector::initialize() {}
void xs::inspector::shutdown() {}
void xs::inspector::render(double) {}
bool xs::inspector::paused() { return false;  }
bool xs::inspector::should_restart() { return false; }
int xs::inspector::notify(xs::inspector::notification_type, const std::string&, float) { return 0; }
void xs::inspector::clear_notification(int id) {}
float xs::inspector::get_game_mouse_x() { return  0.0f; }
float xs::inspector::get_game_mouse_y() { return  0.0f; }

#endif
