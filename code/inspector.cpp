#include "inspector.hpp"

#ifdef INSPECTOR
#include <imgui.h>
#include "imgui_impl.h"
#include <imgui_internal.h>
#include <implot.h>
#include "phosphor_glyph.hpp"
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
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 15.0f;
		constexpr float c_small_font_size = 13.0f;
		constexpr float c_icon_font_size = 17.0f;
		constexpr float c_small_icon_font_size = 14.0f;
		constexpr float c_icon_vertical_offset = 2.0f;
	#else
		constexpr float c_frame_top_bar = 70.0f;
		constexpr float c_frame_bottom_bar = 55.0f;
		constexpr float c_style_scale = 1.0f;

		// Fonts
		constexpr float c_font_size = 16.0f;
		constexpr float c_small_font_size = 12.0f;
		constexpr float c_small_icon_font_size = 10.0f;
		constexpr float c_icon_font_size = 12.0f;
		constexpr float c_icon_vertical_offset = 0.0f;

	#endif

	// Game frame border
	constexpr float c_frame_rounding = 18.0f;

	// Notifications
	constexpr float c_notification_offset_x = -10.0f;
	constexpr float c_notification_offset_y = -10.0f;
	constexpr float c_notification_spacing = 10.0f;

	// Timers
	constexpr float c_reload_ok_timer = 4.0f;
	constexpr float c_tooltip_hover_time = 0.6f;
	constexpr float c_notification_default_time = 3.0f;

	// Phosphor icon font filename (tokens resolved via fileio::get_path)
	constexpr const char* kPhosphorIconFont = "[shared]/fonts/Phosphor.ttf";

	// Status bar text alpha: quiet, as it is always there
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

    enum class zoom_mode { fit, zoom_50, zoom_100, zoom_150, zoom_200 };

    bool game_paused = false;
    bool restart_flag = false;
    float ui_scale = 1.0f;
    zoom_mode current_zoom = zoom_mode::fit;
    bool show_data_registry = false;
    bool show_profiler = false;
    bool show_entities = false;
    bool show_about = false;
    bool show_demo = false;
    bool show_modal = false;
    bool show_inspector = false;
    bool always_on_top = false;
    bool reset_layout = false;
    char* ini_filename = nullptr;

    // Window names. The text after ### is the window's ID, so the icon in front can change without losing its place.
    constexpr const char* c_game_window = "Game";
    constexpr const char* c_data_registry_window = "###Data Registry";
    constexpr const char* c_profiler_window = "###Profiler";
    constexpr const char* c_entities_window = "###Entities";

    // Game viewport bounds (screen space)
    ImVec2 game_viewport_min = {};
    ImVec2 game_viewport_max = {};
    ImVec2 game_viewport_size = {};

    // color identifiers for inspector-specific palette — mirrors xs::inspector::color_id in inspector.hpp
    inline std::array<xs::inspector::color, (size_t)xs::inspector::color_id::Count> inspector_colors;
    inline ImVec4 as_imvec4(const xs::inspector::color& c) { return { c.r, c.g, c.b, c.a }; }

    enum class theme { light, dark };
    theme get_theme();
    theme current_theme = theme::dark;
	void apply_theme(theme t);
	void push_menu_theme(theme t);
	void pop_menu_theme(theme t);
	void dark_theme();
	void light_theme();
	void push_side_panel_theme();
	void pop_side_panel_theme();

	// Helper function to merge Phosphor icons into a font
	ImFont* merge_phosphor_icons(ImFont* base_font, float icon_size, float font_scale, const std::string& font_file = std::string());

	double ok_timer = 0.0f;
	bool next_frame;
	std::vector<notification> notifications;
	ImFont* small_font = nullptr;

    // Forward declarations for render sections
    static void render_dockspace();
    static void render_top_bar();
    static void render_stats_bar(float height);
    static void render_game_viewport();
    static void render_notifications(double dt);
    static void render_data_registry();
    static void render_profiler();
    static void render_entities();
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
#else
	const float font_scale = 1.0f;
	ui_scale = 1.0f;

#endif
	const float font_size = c_font_size;
	const float icon_size = c_icon_font_size;
    
	//ImFontConfig config;
	//config.OversampleH = 8;
	//config.OversampleV = 8;
	
	//auto selawk = fileio::get_path("[shared]/fonts/selawk.ttf");
	//if(!fileio::exists(selawk))
	//{
	//	log::critical("Could not find the font file selawk.ttf at path:{}", selawk);
	//	return;
	//}
	auto selawk_data = fileio::read_binary_file("[shared]/fonts/selawk.ttf");
	// Copy to a heap allocation to ensure it stays valid
	char* selawk_buffer = new char[selawk_data.size()];
	memcpy(selawk_buffer, selawk_data.data(), selawk_data.size());
	auto selawk_font = io.Fonts->AddFontFromMemoryTTF(selawk_buffer, (int)selawk_data.size(), font_size * font_scale, nullptr);
	assert(selawk_font);

	// Merge Phosphor icons into main font
	merge_phosphor_icons(selawk_font, icon_size, font_scale, kPhosphorIconFont);

	// Copy to a heap allocation (again) to ensure it stays valid
	selawk_buffer = new char[selawk_data.size()];
	memcpy(selawk_buffer, selawk_data.data(), selawk_data.size());
	small_font = io.Fonts->AddFontFromMemoryTTF(selawk_buffer, (int)selawk_data.size(), c_small_font_size * font_scale, nullptr);
	assert(small_font);

	// Merge Phosphor icons into small font
	merge_phosphor_icons(small_font, c_small_icon_font_size, font_scale, kPhosphorIconFont);

	const std::string iniPath = fileio::get_path("[user]/imgui.ini");
	log::info("ImGui INI file path: {}", iniPath);

	const char* constStr = iniPath.c_str();
	ini_filename = new char[iniPath.size() + 1];
	strcpy(ini_filename, constStr);
	io.IniFilename = ini_filename;

    auto current_theme = inspector::get_theme();
    apply_theme(current_theme);

	always_on_top = data::get_bool("Editor.AlwaysOnTop", data::type::user);
	if (always_on_top)
		device::toggle_on_top();

	// Restore window states
	show_data_registry = data::get_bool("Editor.ShowDataRegistry", data::type::user);
	show_profiler = data::get_bool("Editor.ShowProfiler", data::type::user);
	show_entities = data::get_bool("Editor.ShowEntities", data::type::user);

	// Restore zoom level
	int saved_zoom = (int)data::get_number("Editor.ZoomLevel", data::type::user);
	if (saved_zoom >= 0 && saved_zoom <= 4)
		current_zoom = static_cast<zoom_mode>(saved_zoom);
}

void xs::inspector::shutdown()
{
	// Save editor state to user settings
	data::set_bool("Editor.ShowDataRegistry", show_data_registry, data::type::user);
	data::set_bool("Editor.ShowProfiler", show_profiler, data::type::user);
	data::set_bool("Editor.ShowEntities", show_entities, data::type::user);
	data::set_number("Editor.ZoomLevel", static_cast<int>(current_zoom), data::type::user);
	data::save_of_type(data::type::user);

	// Explicitly save ImGui settings before shutdown. Skip it if no frame was rendered,
	// as the settings were never loaded and an empty layout would overwrite the saved one.
	const char* ini_path = ImGui::GetIO().IniFilename;
	if (ImGui::GetFrameCount() > 0)
	{
		log::info("Saving ImGui INI settings to: {}", ini_path ? ini_path : "NULL");
		ImGui::SaveIniSettingsToDisk(ini_path);
	}

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

// UI helpers for the bars, sized in multiples of the font size so they follow the UI scale

namespace
{
	// The colors of the play, pause and step buttons: the icon is tinted, and the button is filled with it while on.
	// Pause is blue, not amber: next to green and red it read as a traffic light.
	const ImVec4 c_play_color = {0.25f, 0.70f, 0.35f, 1.0f};
	const ImVec4 c_pause_color = {0.22f, 0.47f, 0.82f, 1.0f};

	// Text on a filled (selected or held) button
	const ImVec4 c_on_accent = {1.0f, 1.0f, 1.0f, 1.0f};

	// What a status bar item's text lights up to when hovered, set by the bar from the theme's text color
	ImVec4 status_hover_text = {1.0f, 1.0f, 1.0f, 1.0f};

	float em(float n) { return ImGui::GetFontSize() * n; }

	// The gap between a group and the top and bottom of the bar, and between the buttons in a group
	float group_inset() { return (std::max)(1.0f, std::round(em(0.15f))); }

	// The gap between the groups in the top bar
	float group_gap() { return std::round(em(0.8f)); }

	// A fixed height window clips its content unless the height also covers its own padding. Whole pixels, as a
	// fractional height leaves a partly covered row on non-HDPI screens.
	float top_bar_height() { return IM_ROUND(ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f); }

	// Slimmer than the top bar, it only holds small items
	float status_bar_height() { return IM_ROUND(ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y); }

	// Starts one of the bars pinned to the top and bottom of the window: full width, square, not dockable or movable
	void begin_bar(const char* name, float y, float height)
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos({viewport->Pos.x, y});
		ImGui::SetNextWindowSize({viewport->Size.x, height});
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::Begin(name, nullptr,
		             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
		             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
		             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();
	}

	// A button drawn by hand at a given place: flat until hovered, filled while held or selected (for toggles). The
	// label is tinted when a tint is given (w > 0), and the fill is the accent unless one is given. A button that cannot
	// be used now has a plain grey label, as a tinted one, even faded, still looks like it does something.
	bool panel_button(const char* id, const char* label, const char* tip, ImVec2 pos, ImVec2 size,
	                  bool enabled = true, bool selected = false, ImVec4 tint = {}, ImVec4 fill = {})
	{
		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImGui::SetCursorScreenPos(pos);
		ImGui::BeginDisabled(!enabled);
		const bool clicked = ImGui::InvisibleButton(id, size);
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		const bool filled = enabled && (selected || ImGui::IsItemActive());
		if (fill.w <= 0.0f)
			fill = as_imvec4(get_color(color_id::Blue));
		const float rounding = ImGui::GetStyle().FrameRounding;
		if (filled)
			draw->AddRectFilled(min, max, ImGui::GetColorU32(fill), rounding);
		else if (ImGui::IsItemHovered())
			draw->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_ButtonHovered), rounding);

		ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
		if (!enabled)
			color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
		else if (filled)
			color = ImGui::GetColorU32(c_on_accent);
		else if (tint.w > 0.0f)
			color = ImGui::GetColorU32(tint);

		// Centered and snapped to whole pixels, as text at a fractional position is blurry on non-HDPI screens
		const char* label_end = ImGui::FindRenderedTextEnd(label);
		const ImVec2 text = ImGui::CalcTextSize(label, label_end);
		draw->AddText({std::round((min.x + max.x - text.x) * 0.5f), std::round((min.y + max.y - text.y) * 0.5f)},
		              color, label, label_end);
		ImGui::EndDisabled();
		if (enabled && tip)
			tooltip(tip);
		return clicked;
	}

	// The height of the buttons in a group, which fill the bar but for the insets
	float group_side() { return ImGui::GetWindowHeight() - 4.0f * group_inset(); }

	// A group of buttons in the top bar, placed left to right. Groups have no background of their own, they are set
	// apart by the soft lines between them (see group_separator).
	struct bar_group
	{
		float left, top, bottom, side, x;

		explicit bar_group(float group_left)
			: left(group_left)
		{
			top = ImGui::GetWindowPos().y + group_inset();
			bottom = ImGui::GetWindowPos().y + ImGui::GetWindowHeight() - group_inset();
			side = group_side();
			x = left + group_inset();
		}

		// The next button, square unless a width is given
		bool button(const char* id, const char* label, const char* tip, bool enabled = true, bool selected = false,
		            ImVec4 tint = {}, ImVec4 fill = {}, float width = 0.0f)
		{
			const ImVec2 pos(x, top + group_inset());
			const ImVec2 size(width > 0.0f ? width : side, side);
			x += size.x + group_inset();
			return panel_button(id, label, tip, pos, size, enabled, selected, tint, fill);
		}

		// Space between buttons that do different kinds of things
		void separate() { x += std::round(group_gap() * 0.5f); }

		// Where the group ends
		float end() const { return x; }
	};

	// A soft vertical line in the gap after a group, as tall as its buttons. Returns where the next group starts.
	float group_separator(float x)
	{
		const float gap = group_gap();
		const float line_x = std::round(x + gap * 0.5f) + 0.5f;  // On a pixel, so the line is crisp
		const float top = ImGui::GetWindowPos().y + 2.0f * group_inset();
		const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		ImGui::GetWindowDrawList()->AddLine({line_x, top}, {line_x, top + group_side()},
		                                    ImGui::GetColorU32({text.x, text.y, text.z, 0.15f}), 1.0f);
		return x + gap;
	}

	// An item for the status bar: an icon and a value in a flat button that lights up under the mouse. It keeps its
	// size as the value changes, so the items after it don't jump around: sample is the widest value, with 'd' for a
	// digit ("ddd.d"). A given width is used as is, and a value too long for it ends in an ellipsis.
	bool status_item(const char* id, const char* icon, const char* value, const char* sample, const char* tip,
	                 float fixed_width = 0.0f)
	{
		// The font's digits can be proportional, so the room is for the widest one
		float digit = 0.0f;
		for (char c = '0'; c <= '9'; c++)
			digit = (std::max)(digit, ImGui::CalcTextSize(&c, &c + 1).x);
		float sample_width = 0.0f;
		for (const char* c = sample; *c; c++)
			sample_width += *c == 'd' ? digit : ImGui::CalcTextSize(c, c + 1).x;

		const ImGuiStyle& style = ImGui::GetStyle();
		const float padding = std::round(em(0.5f));
		const float icon_width = ImGui::CalcTextSize(icon).x + style.ItemInnerSpacing.x;
		const float width = fixed_width > 0.0f
			? fixed_width
			: padding * 2.0f + icon_width + (std::max)(sample_width, ImGui::CalcTextSize(value).x);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, {width, ImGui::GetFrameHeight()});
		const bool hovered = ImGui::IsItemHovered();

		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (hovered)
			draw->AddRectFilled(pos, ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_ButtonHovered), style.FrameRounding);
		if (hovered)
			ImGui::PushStyleColor(ImGuiCol_Text, status_hover_text);
		const float text_y = pos.y + style.FramePadding.y;
		draw->AddText({pos.x + padding, text_y}, ImGui::GetColorU32(ImGuiCol_Text), icon);
		const ImVec2 value_min(pos.x + padding + icon_width, text_y);
		const ImVec2 value_max(pos.x + width - padding, text_y + ImGui::GetFontSize());
		ImGui::RenderTextEllipsis(draw, value_min, value_max, value_max.x, value, nullptr, nullptr);
		if (hovered)
			ImGui::PopStyleColor();

		tooltip(tip);
		return clicked;
	}

	// Play, pause and next frame
	float transport_group(float left)
	{
		bar_group group(left);
		if (group.button("##play", ICON_PH_PLAY, "Play (P)", true, !game_paused, c_play_color, c_play_color))
			game_paused = false;
		if (group.button("##pause", ICON_PH_PAUSE, "Pause (P)", true, game_paused, c_pause_color, c_pause_color))
			game_paused = true;
		if (group.button("##step", ICON_PH_SKIP_FORWARD, "Next frame", game_paused, false, c_play_color, c_play_color))
			next_frame = true;
		return group.end();
	}

	// Reloading the game's scripts and art
	float reload_group(float left)
	{
		const ImVec4 orange = as_imvec4(get_color(color_id::Orange));
		bar_group group(left);
		bool reload_scripts = group.button("##reload_scripts", ICON_PH_SYNC_ALT, "Reload game scripts (F5)", true, false, orange, orange);
		if (xs::input::get_key_once(xs::input::KEY_F5))
			reload_scripts = true;
		if (reload_scripts)
		{
			script::shutdown();
			script::configure();
			script::initialize();
			if (!xs::script::has_error())
				ok_timer = c_reload_ok_timer;
		}

		if (group.button("##reload_art", ICON_PH_IMAGE, "Reload art assets", true, false, orange, orange))
		{
			auto reloaded = render::reload_images();
			next_frame = true;
			if (reloaded == 0)
				notify(notification_type::info, "No images reloaded", c_notification_default_time);
			else
				notify(notification_type::success, to_string(reloaded) + " images reloaded", c_notification_default_time);
		}
		return group.end();
	}

	// The panels that can be opened next to the game
	float windows_group(float left)
	{
		bar_group group(left);
		if (group.button("##data", ICON_PH_BARS, "Data Registry", true, show_data_registry))
			show_data_registry = !show_data_registry;
		if (group.button("##profiler", ICON_PH_PROFILER, "Profiler", true, show_profiler))
			show_profiler = !show_profiler;
		if (script::is_module_loaded("xs/ec") && group.button("##entities", ICON_PH_PUZZLE_CUBE, "Entities", true, show_entities))
			show_entities = !show_entities;
		return group.end();
	}

	// The game view's zoom and the layout, then the window staying on top
	float view_group(float left)
	{
		const char* zoom_labels[] = { "Fit", "50%", "100%", "150%", "200%" };
		bar_group group(left);

		const std::string label = std::string(zoom_labels[(int)current_zoom]) + "  " + ICON_PH_CARET_DOWN + "###view";
		const float width = ImGui::CalcTextSize(label.c_str(), nullptr, true).x + std::round(em(1.0f));
		const float popup_x = group.x;
		const bool open = ImGui::IsPopupOpen("##view_menu");
		if (group.button("##view", label.c_str(), open ? nullptr : "Game viewport zoom and layout", true, open, {}, {}, width))
		{
			ImGui::SetNextWindowPos({popup_x, group.bottom + group_inset()});  // Below the button, not at the mouse
			ImGui::OpenPopup("##view_menu");
		}
		if (ImGui::BeginPopup("##view_menu"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {std::round(em(0.5f)), std::round(em(0.4f))});
			for (int i = 0; i < IM_ARRAYSIZE(zoom_labels); i++)
				if (ImGui::MenuItem(zoom_labels[i], nullptr, (int)current_zoom == i))
					current_zoom = (zoom_mode)i;
			ImGui::Separator();
			if (ImGui::MenuItem((std::string(ICON_PH_LAYOUT) + "  Reset Layout").c_str()))
				reset_layout = true;
			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}

		group.separate();
		if (group.button("##on_top", always_on_top ? ICON_PH_PIN_ON : ICON_PH_PIN_OFF, "Always on Top", true, always_on_top))
		{
			always_on_top = device::toggle_on_top();
			data::set_bool("Editor.AlwaysOnTop", always_on_top, data::type::user);
			data::save_of_type(data::type::user);
		}
#if SHOW_IMGUI_DEMO
		if (group.button("##demo", ICON_PH_WINDOW, "Show ImGui demo window", true, show_demo))
			show_demo = !show_demo;
#endif
		return group.end();
	}

	// A script error and unsaved data, at the right end of the bar (but never over the other groups)
	void alerts_group(float min_left, float right)
	{
		const bool error = xs::script::has_error();
		const bool unsaved = xs::data::has_chages();
		const int count = (error ? 1 : 0) + (unsaved ? 1 : 0);
		if (count == 0)
			return;

		const float width = count * (group_side() + group_inset()) + group_inset();
		bar_group group(std::round((std::max)(min_left, right - width)));
		if (error && group.button("##error", ICON_PH_EXCLAMATION_CIRCLE, "Script Error! Check output. Click to clear.",
		                          true, false, as_imvec4(get_color(color_id::Red)), as_imvec4(get_color(color_id::Red))))
		{
			xs::script::clear_error();
			game_paused = false;
		}
		if (unsaved && group.button("##unsaved", ICON_PH_EXCLAMATION_CIRCLE, "Data has unsaved changes",
		                            true, false, as_imvec4(get_color(color_id::Purple)), as_imvec4(get_color(color_id::Purple))))
			show_data_registry = true;
		group.end();
	}
}

void xs::inspector::search_box(ImGuiTextFilter& filter)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	const bool searching = filter.IsActive();
	const float clear_width = searching ? ImGui::GetFrameHeight() + style.ItemInnerSpacing.x : 0.0f;
	ImGui::SetNextItemWidth((std::max)(1.0f, ImGui::GetContentRegionAvail().x - clear_width));
	const std::string hint = std::string(ICON_PH_SEARCH) + " Search";
	if (ImGui::InputTextWithHint("##search", hint.c_str(), filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf)))
		filter.Build();
	if (searching)
	{
		ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
		if (ImGui::Button(ICON_PH_CLOSE, {ImGui::GetFrameHeight(), 0.0f}))
			filter.Clear();
		tooltip("Clear search");
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
	
	ImGui_Impl_NewFrame();
    ImGui::NewFrame();
	ok_timer -= dt;
    auto current_theme = get_theme();
	restart_flag = false;
	// The status bar is pinned to the bottom of the window, and the docked windows fill what is left above it, so the
	// panels next to the game go all the way to the top. The top bar is part of the Game panel (see
	// render_game_viewport). Measured with the font it is drawn with.
	ImGui::PushFont(small_font);
	const float status_height = status_bar_height();
	ImGui::PopFont();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	viewport->WorkSize.y -= status_height;

	render_dockspace();
	render_stats_bar(status_height);
	render_game_viewport();
	render_notifications(dt);
	render_data_registry();
	render_profiler();
	render_entities();

	if(show_demo)
	{
		ImGui::ShowDemoWindow(&show_demo);
	}

	ImGui::Render();
	ImGui_Impl_RenderDrawData(ImGui::GetDrawData());
}

// Render section implementations

// The default layout: the game in the middle, and the panels that are open docked to its right
static void build_layout(ImGuiID dockspace_id)
{
	ImGui::DockBuilderRemoveNode(dockspace_id);  // Undocks everything, rebuilt below
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

	std::vector<const char*> panels;
	if (show_data_registry) panels.push_back(c_data_registry_window);
	if (show_profiler) panels.push_back(c_profiler_window);
	if (show_entities) panels.push_back(c_entities_window);

	ImGuiID center = dockspace_id;
	if (!panels.empty())
	{
		const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.3f, nullptr, &center);
		for (const char* panel : panels)
			ImGui::DockBuilderDockWindow(panel, right);
	}
	ImGui::DockBuilderDockWindow(c_game_window, center);
	if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(center))
		node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
	ImGui::DockBuilderFinish(dockspace_id);
}

static void xs::inspector::render_dockspace()
{
	// Fills the area between the two bars. Its own window draws no background, the panels docked in it do.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("##DockHost", nullptr,
	             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking |
	             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
	             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleVar(3);

	// Laid out the first time (nothing in imgui.ini yet) and on Reset Layout. The layout itself is in imgui.ini, so a
	// missing or deleted file gives the default layout again.
	const ImGuiID dockspace_id = ImGui::GetID("Layout");
	if (reset_layout || ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
	{
		build_layout(dockspace_id);
		reset_layout = false;
	}
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();
}

static void xs::inspector::render_top_bar()
{
	// A strip across the top of the Game panel, on the panel's own color
	auto current_theme = get_theme();
	push_menu_theme(current_theme);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	ImGui::BeginChild("##TopBar", {0.0f, top_bar_height()}, ImGuiChildFlags_AlwaysUseWindowPadding,
	                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);

	// A script error pauses the game until it is cleared
	if (xs::script::has_error())
		game_paused = true;

	// The groups from the left edge of the bar, with a soft line between them
	const float bar_left = ImGui::GetWindowPos().x;
	const float bar_right = bar_left + ImGui::GetWindowWidth();
	const float edge = ImGui::GetStyle().WindowPadding.x;

	float x = std::round(bar_left + edge - group_inset());  // The buttons, not their inset, line up with the edge
	x = group_separator(transport_group(x));
	if (xs::get_run_mode() == run_mode::development)
	{
		x = group_separator(reload_group(x));
		x = group_separator(windows_group(x));
	}
	x = view_group(x);

	// What needs attention, at the right end of the bar, so it doesn't move the groups when it comes and goes
	alerts_group(x + group_gap(), bar_right - edge);

	ImGui::EndChild();
	ImGui::PopStyleColor();
	pop_menu_theme(current_theme);
}

static void xs::inspector::render_stats_bar(float height)
{
	ImGui::PushFont(small_font);

	// Quiet text, as it is always there. An item lights up to the theme's own text color while hovered.
	const ImVec4 base_text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
	status_hover_text = base_text;
	ImGui::PushStyleColor(ImGuiCol_Text, {base_text.x, base_text.y, base_text.z, c_status_text_alpha});
	const float padding_y = std::round((height - ImGui::GetFrameHeight()) * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {ImGui::GetStyle().WindowPadding.x, padding_y});
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	begin_bar("##StatusBar", viewport->Pos.y + viewport->Size.y - height, height);

	// Frames per second and frame time are the average of the last second, and they hold that value until the next
	// second is over: even a sliding average flickers, as its last digit changes every frame. Until the first second
	// is over, they show what there is.
	static float accumulated_time = 0.0f;
	static int accumulated_frames = 0;
	static float average_frame_time = 0.0f;
	accumulated_time += ImGui::GetIO().DeltaTime;
	accumulated_frames++;
	if (accumulated_time >= 1.0f || average_frame_time == 0.0f)
	{
		average_frame_time = accumulated_time / (float)accumulated_frames;
		if (accumulated_time >= 1.0f)
		{
			accumulated_time = 0.0f;
			accumulated_frames = 0;
		}
	}

	const float gap = std::round(ImGui::GetFontSize() * 0.15f);
	auto stats = render::get_stats();
	char value[64];

	snprintf(value, sizeof(value), "%.1f", average_frame_time > 0.0f ? 1.0f / average_frame_time : 0.0f);
	status_item("##stat_fps", ICON_PH_MONITOR, value, "ddd.d", "Frames per second (average, updated every second)");
	ImGui::SameLine(0.0f, gap);

	snprintf(value, sizeof(value), "%.2f ms", average_frame_time * 1000.0f);
	status_item("##stat_frame", ICON_PH_CLOCK, value, "dd.dd ms", "Frame time (average, updated every second)");
	ImGui::SameLine(0.0f, gap);

	const auto mb = script::get_bytes_allocated() / (1024.0f * 1024.0f);
	snprintf(value, sizeof(value), "%.1f MB", mb);
	status_item("##stat_mem", ICON_PH_MEMORY, value, "ddd.d MB", "Memory allocated by Wren VM (MB)");
	ImGui::SameLine(0.0f, gap);

	snprintf(value, sizeof(value), "%d", (int)stats.draw_calls);
	status_item("##stat_dc", ICON_PH_IMAGE_PEN, value, "dddd", "Draw calls this frame");
	ImGui::SameLine(0.0f, gap);

	snprintf(value, sizeof(value), "%d", (int)stats.sprites);
	status_item("##stat_sprites", ICON_PH_IMAGE_FRAME, value, "ddddd", "Sprites drawn this frame");
	ImGui::SameLine(0.0f, gap);

	snprintf(value, sizeof(value), "%d", (int)stats.textures);
	status_item("##stat_textures", ICON_PH_IMAGES, value, "ddd", "Number of textures loaded in GPU memory");
	ImGui::SameLine(0.0f, gap);

	const auto ver = version::get_version_string(false, true, true);
	if (status_item("##stat_version", ICON_PH_TAG, ver.c_str(), ver.c_str(), "xs version (click to copy)"))
	{
		ImGui::SetClipboardText(ver.c_str());
		notify(notification_type::info, "Version copied to clipboard", c_notification_default_time);
	}
	ImGui::SameLine(0.0f, gap);

	// The game folder, in the rest of the bar
	const std::string path = fileio::absolute("[game]");
	const float path_width = (std::max)(1.0f, ImGui::GetContentRegionAvail().x);
	if (status_item("##stat_path", ICON_PH_FOLDER, path.c_str(), "", "Click to copy the game folder path to clipboard", path_width))
	{
		ImGui::SetClipboardText(path.c_str());
		notify(notification_type::info, "Path copied to clipboard", c_notification_default_time);
	}

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopFont();
}

static void xs::inspector::render_game_viewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin(c_game_window, nullptr,
		             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
		             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		// The top bar, then the game below it. Only the game scrolls (in the fixed zoom modes), the bar stays put.
		render_top_bar();
		ImGuiWindowFlags view_flags = ImGuiWindowFlags_None;
		if (current_zoom != zoom_mode::fit)
			view_flags |= ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		ImGui::BeginChild("##GameView", {0.0f, 0.0f}, ImGuiChildFlags_None, view_flags);

		// Get the game render target texture
		auto texture = xs::render::get_render_target_texture();

		// Get available content region (respects window padding/borders)
		ImVec2 available_region = ImGui::GetContentRegionAvail();

		// Calculate aspect ratio
		float game_width = (float)xs::configuration::width();
		float game_height = (float)xs::configuration::height();
		float aspect_ratio = game_width / game_height;
		// Natural display size is the multiplied resolution
		float display_width  = game_width  * xs::configuration::multiplier();
		float display_height = game_height * xs::configuration::multiplier();

		// Calculate size based on zoom mode
		ImVec2 image_size;
		if (current_zoom == zoom_mode::fit)
		{
			// Best fit - maintain an aspect ratio
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
			// Fixed zoom percentages relative to the multiplied (natural) resolution
			float zoom_factor = 1.0f;
			switch (current_zoom)
			{
				case zoom_mode::zoom_50:  zoom_factor = 0.5f;  break;
				case zoom_mode::zoom_100: zoom_factor = 1.0f;  break;
				case zoom_mode::zoom_150: zoom_factor = 1.5f;  break;
				case zoom_mode::zoom_200: zoom_factor = 2.0f;  break;
				default: zoom_factor = 1.0f; break;
			}
			image_size.x = display_width  * zoom_factor;
			image_size.y = display_height * zoom_factor;
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

		// Draw image using DrawList, respecting the project's texture filter setting
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		bool use_linear = data::get_bool("Render.TextureFilter", data::type::project);
		auto& pio = ImGui::GetPlatformIO();
		draw_list->AddCallback(use_linear ? pio.DrawCallback_SetSamplerLinear : pio.DrawCallback_SetSamplerNearest, nullptr);
#if defined(PLATFORM_APPLE)
		draw_list->AddImage((ImTextureID)texture, screen_pos, screen_pos_max,
		                    ImVec2(0, 0), ImVec2(1, 1));
#elif defined(PLATFORM_OPENGL)
		draw_list->AddImage((ImTextureID)texture, screen_pos, screen_pos_max,
		                    ImVec2(0, 1), ImVec2(1, 0));
#endif
		draw_list->AddCallback(pio.DrawCallback_SetSamplerLinear, nullptr);

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
	ImGui::EndChild();
	ImGui::PopStyleColor();
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
			xs::inspector::color color;
			string icon;
			switch (n.type)
			{
			case notification_type::info:
				color = get_color(color_id::Blue);
				icon = ICON_PH_INFO_CIRCLE;
				break;
			case notification_type::success:
				color = get_color(color_id::Green);
				icon = ICON_PH_CHECK_CIRCLE;
				break;
			case notification_type::warning:
				color = get_color(color_id::Orange);
				icon = ICON_PH_EXCLAMATION_CIRCLE;
				break;
			case notification_type::error:
				color = get_color(color_id::Red);
				icon = ICON_PH_EXCLAMATION_CIRCLE;
				break;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, as_imvec4(color));
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

static void xs::inspector::render_data_registry()
{
	if (!show_data_registry)
		return;

	push_side_panel_theme();
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	// Disable window menu button in tab bar
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;

	ImGui::SetNextWindowClass(&window_class);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, {0,0,0, 0.0f});
	if (ImGui::Begin(
		(std::string(ICON_PH_BARS) + " Data Registry" + c_data_registry_window).c_str(),
		nullptr, flags))
	{
		data::inspect();
	}
	ImGui::PopStyleColor();
	ImGui::End();
	pop_side_panel_theme();
}

static void xs::inspector::render_profiler()
{
	if (!show_profiler)
		return;

	push_side_panel_theme();
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	// Disable window menu button in tab bar
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
	ImGui::SetNextWindowClass(&window_class);

	ImGui::Begin((std::string(ICON_PH_PROFILER) + " Profiler" + c_profiler_window).c_str(), nullptr, flags);
	profiler::inspect();
	ImGui::End();
	pop_side_panel_theme();
}

static void xs::inspector::render_entities()
{
	if (!show_entities)
		return;

	static ImGuiTextFilter entity_filter;

	push_side_panel_theme();
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	// Disable window menu button in tab bar
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
	ImGui::SetNextWindowClass(&window_class);

	if (ImGui::Begin((std::string(ICON_PH_PUZZLE_CUBE) + " Entities" + c_entities_window).c_str(),
		nullptr, flags))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2 * c_style_scale);

		search_box(entity_filter);

		// Pass filter to Wren - it handles entity list, filtering, selection, and components
		script::ec_inspect(entity_filter.InputBuf);

		ImGui::PopStyleVar();
	}
	ImGui::End();
	pop_side_panel_theme();
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

xs::inspector::color xs::inspector::get_color(color_id id)
{
	return inspector_colors[(size_t)id];
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
        dark_theme();
        break;
    case theme::light:
        light_theme();
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
	style.TabBarOverlineSize = 2 * c_style_scale;

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
	style.TabRounding = 0 * c_style_scale;

	// What cannot be used now: clearly off, not just a little dim
	style.DisabledAlpha = 0.35f;

	// Other
	style.LogSliderDeadzone = 4 * c_style_scale;
}

void xs::inspector::dark_theme()
{
	// Try to get system colors for titlebar and window background
	ImVec4 windowBg = {0.10f, 0.10f, 0.10f, 1.00f};
	ImVec4 titleBg = {0.00f, 0.00f, 0.00f, 1.00f};
	ImVec4 titleBgActive = {0.06f, 0.06f, 0.06f, 1.00f};
	
#if defined(PLATFORM_PC)
	// Windows dark theme: slightly lighter dark gray
	windowBg = {0.12f, 0.12f, 0.12f, 1.00f};
	titleBg = {0.10f, 0.10f, 0.10f, 1.00f};
	titleBgActive = {0.15f, 0.15f, 0.15f, 1.00f};
#elif defined(PLATFORM_APPLE)
	// macOS dark theme: deeper blacks with subtle warmth
    windowBg = ImColor(35, 34, 48, 255);
    titleBg = windowBg;
	titleBgActive = {0.18f, 0.18f, 0.18f, 1.00f};
#endif

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]                   = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TextDisabled]           = {0.50f, 0.50f, 0.50f, 1.00f};
	colors[ImGuiCol_WindowBg]               = {0.12f, 0.12f, 0.12f, 1.00f};
	colors[ImGuiCol_ChildBg]                = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_PopupBg]                = {0.31f, 0.31f, 0.31f, 1.00f};
	colors[ImGuiCol_Border]                 = {0.19f, 0.19f, 0.19f, 0.00f};
	colors[ImGuiCol_BorderShadow]           = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_FrameBg]                = {1.00f, 1.00f, 1.00f, 0.06f};
	colors[ImGuiCol_FrameBgHovered]         = {1.00f, 1.00f, 1.00f, 0.20f};
	colors[ImGuiCol_FrameBgActive]          = {1.00f, 1.00f, 1.00f, 0.29f};
	colors[ImGuiCol_TitleBg]                = {0.10f, 0.10f, 0.10f, 1.00f};
	colors[ImGuiCol_TitleBgActive]          = {0.15f, 0.15f, 0.15f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed]       = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_MenuBarBg]              = {0.14f, 0.14f, 0.14f, 1.00f};
	colors[ImGuiCol_ScrollbarBg]            = {1.00f, 1.00f, 1.00f, 0.00f};
	colors[ImGuiCol_ScrollbarGrab]          = {1.00f, 1.00f, 1.00f, 0.10f};
	colors[ImGuiCol_ScrollbarGrabHovered]   = {1.00f, 1.00f, 1.00f, 0.20f};
	colors[ImGuiCol_ScrollbarGrabActive]    = {1.00f, 1.00f, 1.00f, 0.29f};
	colors[ImGuiCol_CheckMark]              = {0.33f, 0.67f, 0.86f, 1.00f};
	colors[ImGuiCol_SliderGrab]             = {0.34f, 0.34f, 0.34f, 0.54f};
	colors[ImGuiCol_SliderGrabActive]       = {0.56f, 0.56f, 0.56f, 0.54f};
	colors[ImGuiCol_Button]                 = {1.00f, 1.00f, 1.00f, 0.00f};
	colors[ImGuiCol_ButtonHovered]          = {1.00f, 1.00f, 1.00f, 0.10f};
	colors[ImGuiCol_ButtonActive]           = {1.00f, 1.00f, 1.00f, 0.20f};
	colors[ImGuiCol_Header]                 = {0.00f, 0.00f, 0.00f, 0.52f};
	colors[ImGuiCol_HeaderHovered]          = {0.00f, 0.00f, 0.00f, 0.36f};
	colors[ImGuiCol_HeaderActive]           = {0.20f, 0.22f, 0.23f, 0.33f};
	colors[ImGuiCol_Separator]              = {0.28f, 0.28f, 0.28f, 1.00f};
	colors[ImGuiCol_SeparatorHovered]       = {0.44f, 0.44f, 0.44f, 0.50f};
	colors[ImGuiCol_SeparatorActive]        = {0.40f, 0.44f, 0.47f, 1.00f};
	colors[ImGuiCol_ResizeGrip]             = {0.28f, 0.28f, 0.28f, 0.00f};
	colors[ImGuiCol_ResizeGripHovered]      = {0.44f, 0.44f, 0.44f, 0.50f};
	colors[ImGuiCol_ResizeGripActive]       = {0.40f, 0.44f, 0.47f, 1.00f};
	colors[ImGuiCol_InputTextCursor]        = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TabHovered]             = {1.00f, 1.00f, 1.00f, 0.10f};
	colors[ImGuiCol_Tab]                    = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TabSelected]            = {1.00f, 1.00f, 1.00f, 0.00f};
	colors[ImGuiCol_TabSelectedOverline]    = {0.33f, 0.67f, 0.86f, 1.00f};
	colors[ImGuiCol_TabDimmed]              = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TabDimmedSelected]      = {0.14f, 0.14f, 0.14f, 1.00f};
	colors[ImGuiCol_TabDimmedSelectedOverline]  = {0.50f, 0.50f, 0.50f, 0.00f};
	colors[ImGuiCol_DockingPreview]         = {0.33f, 0.67f, 0.86f, 0.70f};
	colors[ImGuiCol_DockingEmptyBg]         = {0.20f, 0.20f, 0.20f, 1.00f};
	colors[ImGuiCol_PlotLines]              = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered]       = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogram]          = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered]   = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TableHeaderBg]          = {0.19f, 0.19f, 0.20f, 1.00f};
	colors[ImGuiCol_TableBorderStrong]      = {0.31f, 0.31f, 0.35f, 1.00f};
	colors[ImGuiCol_TableBorderLight]       = {0.23f, 0.23f, 0.25f, 1.00f};
	colors[ImGuiCol_TableRowBg]             = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBgAlt]          = {1.00f, 1.00f, 1.00f, 0.06f};
	colors[ImGuiCol_TextLink]               = {0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_TextSelectedBg]         = {0.20f, 0.22f, 0.23f, 1.00f};
	colors[ImGuiCol_TreeLines]              = {0.43f, 0.43f, 0.50f, 0.50f};
	colors[ImGuiCol_DragDropTarget]         = {0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_DragDropTargetBg]       = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_UnsavedMarker]          = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_NavCursor]              = {1.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_NavWindowingHighlight]  = {1.00f, 0.00f, 0.00f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg]      = {1.00f, 0.00f, 0.00f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg]       = {0.00f, 0.00f, 0.00f, 0.75f};
	colors[ImGuiCol_ChildBg]                = {0.12f, 0.12f, 0.12f, 1.00f};
	colors[ImGuiCol_Header]                 = {1.00f, 1.00f, 1.00f, 0.14f};
	colors[ImGuiCol_HeaderHovered]          = {1.00f, 1.00f, 1.00f, 0.21f};
	colors[ImGuiCol_HeaderActive]           = {1.00f, 1.00f, 1.00f, 0.38f};

	colors[ImGuiCol_WindowBg] = windowBg;
	colors[ImGuiCol_TitleBg] = titleBg;
	colors[ImGuiCol_TitleBgActive] = titleBgActive;
	
	apply_common_style();

	// Populate inspector-specific palette for dark theme
    inspector_colors[(size_t)color_id::Green] = {0.2f, 0.8f, 0.2f, 1.0f};
    inspector_colors[(size_t)color_id::Blue] = {0.3f, 0.6f, 0.9f, 1.0f};
    inspector_colors[(size_t)color_id::Orange] = {0.9f, 0.6f, 0.2f, 1.0f};
    inspector_colors[(size_t)color_id::Purple] = {0.5f, 0.4f, 0.8f, 1.0f};
    inspector_colors[(size_t)color_id::Pink] = {0.8f, 0.3f, 0.7f, 1.0f};
    inspector_colors[(size_t)color_id::Gray] = {0.5f, 0.5f, 0.5f, 1.0f};
    inspector_colors[(size_t)color_id::Red] = {0.9f, 0.2f, 0.2f, 1.0f};
}

void xs::inspector::light_theme()
{
	// Try to get system colors for titlebar and window backgrounds
	ImVec4 windowBg = {0.94f, 0.94f, 0.94f, 1.00f};
	ImVec4 titleBg = {0.96f, 0.96f, 0.96f, 1.00f};
	ImVec4 titleBgActive = {0.82f, 0.82f, 0.82f, 1.00f};
	
#if defined(PLATFORM_PC)
	// Windows light theme: lighter gray for inactive, slightly darker for active
	windowBg = {0.98f, 0.98f, 0.98f, 1.00f};
	titleBg = {0.98f, 0.98f, 0.98f, 1.00f};
	titleBgActive = {0.95f, 0.95f, 0.95f, 1.00f};
#elif defined(PLATFORM_APPLE)
	// macOS light theme: very light gray, almost white
	windowBg = {1.0f, 1.0f, 1.0f, 1.00f};
	titleBg = {0.98f, 0.98f, 0.98f, 1.00f};
	titleBgActive = {0.96f, 0.96f, 0.96f, 1.00f};
#endif

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text]                   = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TextDisabled]           = {0.60f, 0.60f, 0.60f, 1.00f};
	colors[ImGuiCol_WindowBg]               = {0.98f, 0.98f, 0.98f, 1.00f};
	colors[ImGuiCol_ChildBg]                = {0.00f, 0.00f, 0.00f, 0.04f};
	colors[ImGuiCol_PopupBg]                = {0.91f, 0.91f, 0.91f, 1.00f};
	colors[ImGuiCol_Border]                 = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_BorderShadow]           = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_FrameBg]                = {0.90f, 0.90f, 0.90f, 1.00f};
	colors[ImGuiCol_FrameBgHovered]         = {0.74f, 0.74f, 0.74f, 1.00f};
	colors[ImGuiCol_FrameBgActive]          = {0.55f, 0.55f, 0.55f, 1.00f};
	colors[ImGuiCol_TitleBg]                = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TitleBgActive]          = {1.00f, 1.00f, 1.00f, 1.00f};
	colors[ImGuiCol_TitleBgCollapsed]       = {1.00f, 1.00f, 1.00f, 0.51f};
	colors[ImGuiCol_MenuBarBg]              = {0.86f, 0.86f, 0.86f, 1.00f};
	colors[ImGuiCol_ScrollbarBg]            = {0.98f, 0.98f, 0.98f, 0.00f};
	colors[ImGuiCol_ScrollbarGrab]          = {0.69f, 0.69f, 0.69f, 0.80f};
	colors[ImGuiCol_ScrollbarGrabHovered]   = {0.49f, 0.49f, 0.49f, 0.80f};
	colors[ImGuiCol_ScrollbarGrabActive]    = {0.49f, 0.49f, 0.49f, 1.00f};
	colors[ImGuiCol_CheckMark]              = {0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_SliderGrab]             = {0.26f, 0.59f, 0.98f, 0.78f};
	colors[ImGuiCol_SliderGrabActive]       = {0.46f, 0.54f, 0.80f, 0.60f};
	colors[ImGuiCol_Button]                 = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_ButtonHovered]          = {0.00f, 0.00f, 0.00f, 0.08f};
	colors[ImGuiCol_ButtonActive]           = {0.00f, 0.00f, 0.00f, 0.18f};
	colors[ImGuiCol_Header]                 = {0.41f, 0.41f, 0.41f, 0.31f};
	colors[ImGuiCol_HeaderHovered]          = {0.41f, 0.41f, 0.41f, 0.48f};
	colors[ImGuiCol_HeaderActive]           = {0.41f, 0.41f, 0.41f, 0.51f};
	colors[ImGuiCol_Separator]              = {0.39f, 0.39f, 0.39f, 0.62f};
	colors[ImGuiCol_SeparatorHovered]       = {0.14f, 0.44f, 0.80f, 0.78f};
	colors[ImGuiCol_SeparatorActive]        = {0.14f, 0.44f, 0.80f, 1.00f};
	colors[ImGuiCol_ResizeGrip]             = {0.80f, 0.80f, 0.80f, 0.56f};
	colors[ImGuiCol_ResizeGripHovered]      = {0.26f, 0.59f, 0.98f, 0.67f};
	colors[ImGuiCol_ResizeGripActive]       = {0.26f, 0.59f, 0.98f, 0.95f};
	colors[ImGuiCol_InputTextCursor]        = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_TabHovered]             = {0.00f, 0.00f, 0.00f, 0.16f};
	colors[ImGuiCol_Tab]                    = {1.00f, 1.00f, 1.00f, 0.00f};
	colors[ImGuiCol_TabSelected]            = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TabSelectedOverline]    = {0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_TabDimmed]              = {0.92f, 0.93f, 0.94f, 0.99f};
	colors[ImGuiCol_TabDimmedSelected]      = {1.00f, 1.00f, 1.00f, 0.99f};
	colors[ImGuiCol_TabDimmedSelectedOverline]  = {0.50f, 0.50f, 0.50f, 0.00f};
	colors[ImGuiCol_DockingPreview]         = {0.26f, 0.59f, 0.98f, 0.70f};
	colors[ImGuiCol_DockingEmptyBg]         = {0.20f, 0.20f, 0.20f, 1.00f};
	colors[ImGuiCol_PlotLines]              = {0.39f, 0.39f, 0.39f, 1.00f};
	colors[ImGuiCol_PlotLinesHovered]       = {1.00f, 0.43f, 0.35f, 1.00f};
	colors[ImGuiCol_PlotHistogram]          = {0.90f, 0.70f, 0.00f, 1.00f};
	colors[ImGuiCol_PlotHistogramHovered]   = {1.00f, 0.45f, 0.00f, 1.00f};
	colors[ImGuiCol_TableHeaderBg]          = {0.19f, 0.19f, 0.20f, 1.00f};
	colors[ImGuiCol_TableBorderStrong]      = {0.31f, 0.31f, 0.35f, 1.00f};
	colors[ImGuiCol_TableBorderLight]       = {0.23f, 0.23f, 0.25f, 1.00f};
	colors[ImGuiCol_TableRowBg]             = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_TableRowBgAlt]          = {1.00f, 1.00f, 1.00f, 0.06f};
	colors[ImGuiCol_TextLink]               = {0.26f, 0.59f, 0.98f, 1.00f};
	colors[ImGuiCol_TextSelectedBg]         = {0.26f, 0.59f, 0.98f, 0.35f};
	colors[ImGuiCol_TreeLines]              = {0.43f, 0.43f, 0.50f, 0.50f};
	colors[ImGuiCol_DragDropTarget]         = {0.26f, 0.59f, 0.98f, 0.95f};
	colors[ImGuiCol_DragDropTargetBg]       = {0.00f, 0.00f, 0.00f, 0.00f};
	colors[ImGuiCol_UnsavedMarker]          = {0.00f, 0.00f, 0.00f, 1.00f};
	colors[ImGuiCol_NavCursor]              = {0.26f, 0.59f, 0.98f, 0.80f};
	colors[ImGuiCol_NavWindowingHighlight]  = {0.70f, 0.70f, 0.70f, 0.70f};
	colors[ImGuiCol_NavWindowingDimBg]      = {0.20f, 0.20f, 0.20f, 0.20f};
	colors[ImGuiCol_ModalWindowDimBg]       = {0.20f, 0.20f, 0.20f, 0.35f};



	apply_common_style();

	// Populate inspector-specific palette for light theme
    inspector_colors[(size_t)color_id::Green] = {0.07f, 0.6f, 0.07f, 1.0f};
    inspector_colors[(size_t)color_id::Blue] = {0.07f, 0.45f, 0.8f, 1.0f};
    inspector_colors[(size_t)color_id::Orange] = {0.8f, 0.5f, 0.08f, 1.0f};
    inspector_colors[(size_t)color_id::Purple] = {0.45f, 0.35f, 0.75f, 1.0f};
    inspector_colors[(size_t)color_id::Pink] = {0.75f, 0.28f, 0.65f, 1.0f};
    inspector_colors[(size_t)color_id::Gray] = {0.35f, 0.35f, 0.35f, 1.0f};
    inspector_colors[(size_t)color_id::Red] = {0.85f, 0.15f, 0.15f, 1.0f};
}

void inspector::push_side_panel_theme()
{
	ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	if (current_theme == theme::dark)
		bg = {bg.x + 0.03f, bg.y + 0.03f, bg.z + 0.03f, bg.w};
	else
		bg = {bg.x * 0.97f, bg.y * 0.97f, bg.z * 0.97f, bg.w};
	ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
}

void inspector::pop_side_panel_theme()
{
	ImGui::PopStyleColor();
}

// Add missing helper implementations
ImFont* xs::inspector::merge_phosphor_icons(ImFont* base_font, float icon_size, float font_scale, const std::string& font_file)
{
    auto& io = ImGui::GetIO();
    const ImWchar* phosphor_icons_ranges = xs::tools::get_phosphor_glyph_ranges();

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphOffset = ImVec2(0.0f, c_icon_vertical_offset * font_scale * ui_scale);
    config.OversampleH = 8;
    config.OversampleV = 8;

    std::string token = font_file.empty() ? std::string(kPhosphorIconFont) : font_file;
    std::string phosphor_font = fileio::get_path(token);
    if(!fileio::exists(phosphor_font))
    {
        log::critical("Could not find the phosphor icon font at path:{}", phosphor_font);
        return base_font;
    }

    auto phosphor_font_data = fileio::read_binary_file(phosphor_font);
    char* phosphor_font_buffer = new char[phosphor_font_data.size()];
    memcpy(phosphor_font_buffer, phosphor_font_data.data(), phosphor_font_data.size());

    auto phosphor_icons_font = io.Fonts->AddFontFromMemoryTTF(
        phosphor_font_buffer,
        (int)phosphor_font_data.size(),
        icon_size * font_scale,
        &config,
        phosphor_icons_ranges);
    assert(phosphor_icons_font != nullptr);

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
void xs::inspector::search_box(ImGuiTextFilter&) {}
float xs::inspector::get_game_mouse_x() { return  0.0f; }
float xs::inspector::get_game_mouse_y() { return  0.0f; }
xs::inspector::color xs::inspector::get_color(color_id) { return { 1.0f, 1.0f, 1.0f, 1.0f }; }

#endif
