#pragma once
#include <map>
#include <string_view>

namespace xs::inspector::ui_colors
{
	// UI color configuration
	void initialize();
	void shutdown();

	// Push a color to the current stack
	void push_color(std::string_view id);
	// Pop a color from the current stack
	void pop_color(std::string_view id = "");

	// Set color for an ID
	void set_color(std::string_view id, const ImVec4& color);

	// Get color for an ID
	const ImVec4& get_color(std::string_view id);

	// Predefined color IDs
	constexpr std::string_view ButtonColor = "button_color";
	constexpr std::string_view ButtonHoverColor = "button_hover_color";
	constexpr std::string_view ButtonActiveColor = "button_active_color";
	constexpr std::string_view HeaderColor = "header_color";
	constexpr std::string_view HeaderHoverColor = "header_hover_color";
	constexpr std::string_view HeaderActiveColor = "header_active_color";
	constexpr std::string_view SeparatorColor = "separator_color";
	constexpr std::string_view SeparatorHoverColor = "separator_hover_color";
	constexpr std::string_view SeparatorActiveColor = "separator_active_color";
	constexpr std::string_view TextColor = "text_color";
	constexpr std::string_view TextDisabledColor = "text_disabled_color";
	constexpr std::string_view WindowBgColor = "window_bg_color";
	constexpr std::string_view ChildBgColor = "child_bg_color";
	constexpr std::string_view PopupBgColor = "popup_bg_color";
	constexpr std::string_view BorderColor = "border_color";
	constexpr std::string_view BorderShadowColor = "border_shadow_color";
	constexpr std::string_view FrameBgColor = "frame_bg_color";
	constexpr std::string_view FrameBgHoverColor = "frame_bg_hover_color";
	constexpr std::string_view FrameBgActiveColor = "frame_bg_active_color";
	constexpr std::string_view TitleBgColor = "title_bg_color";
	constexpr std::string_view TitleBgActiveColor = "title_bg_active_color";
	constexpr std::string_view TitleBgCollapsedColor = "title_bg_collapsed_color";
	constexpr std::string_view MenuBarBgColor = "menu_bar_bg_color";
	constexpr std::string_view ScrollbarBgColor = "scrollbar_bg_color";
	constexpr std::string_view ScrollbarGrabColor = "scrollbar_grab_color";
	constexpr std::string_view ScrollbarGrabHoverColor = "scrollbar_grab_hover_color";
	constexpr std::string_view ScrollbarGrabActiveColor = "scrollbar_grab_active_color";
	constexpr std::string_view SliderGrabColor = "slider_grab_color";
	constexpr std::string_view SliderGrabActiveColor = "slider_grab_active_color";
	constexpr std::string_view CheckMarkColor = "check_mark_color";
	constexpr std::string_view DragDropTargetColor = "drag_drop_target_color";
	constexpr std::string_view NavHighlightColor = "nav_highlight_color";
	constexpr std::string_view NavWindowingHighlightColor = "nav_windowing_highlight_color";
	constexpr std::string_view NavWindowingDimBgColor = "nav_windowing_dim_bg_color";
	constexpr std::string_view ModalWindowDimBgColor = "modal_window_dim_bg_color";

	// Initialize default colors
	void set_default_colors();
};

constexpr auto& operator<<(ImDrawList* draw_list, const xs::inspector::ui_colors::ColorPush& color_push)
{
	if(color_push.id.empty())
		return *draw_list;

	draw_list->PushStyleColor(color_push.idx, xs::inspector::ui_colors::get_color(color_push.id));
	return *draw_list;
}

constexpr auto& operator<<(ImDrawList* draw_list, const xs::inspector::ui_colors::ColorPop&)
{
	draw_list->PopStyleColor();
	return *draw_list;
}

namespace xs::inspector
{
    // UI color map (keyed by name to allow per-theme tweaks)
    inline std::map<std::string, ImVec4> ui_colors;

    // Populate ui_colors from current ImGui style defaults. Call after theme colors are set.
    inline void update_ui_colors()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ui_colors["button"] = style.Colors[ImGuiCol_Button];
        ui_colors["button_hover"] = style.Colors[ImGuiCol_ButtonHovered];
        ui_colors["button_active"] = style.Colors[ImGuiCol_ButtonActive];

        // Menu buttons (we prefer transparent by default)
        ui_colors["menu_button"] = ImVec4(0,0,0,0);
        ui_colors["menu_button_hover"] = style.Colors[ImGuiCol_ButtonHovered];
        ui_colors["menu_button_active"] = style.Colors[ImGuiCol_ButtonActive];

        // Status bar buttons (default to transparent look)
        ui_colors["status_button"] = ImVec4(0,0,0,0);
        ui_colors["status_button_hover"] = ImVec4(0,0,0,0.08f);
        ui_colors["status_button_active"] = ImVec4(0,0,0,0.12f);

        // Header colors (used for selectable styling)
        ui_colors["header"] = style.Colors[ImGuiCol_Header];
        ui_colors["header_hover"] = style.Colors[ImGuiCol_HeaderHovered];
        ui_colors["header_active"] = style.Colors[ImGuiCol_HeaderActive];
    }

    // ...existing functions...
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

    // Update ui_colors after theme was applied so per-theme tweaks are available
    update_ui_colors();

    // ...existing code that scales style ...
#if defined(PLATFORM_PC)
    auto& style = ImGui::GetStyle();
    float scale = (float)device::hdpi_scaling();
    style.ScaleAllSizes(scale);
#endif
}

void xs::inspector::push_menu_theme(theme t)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ui_colors.count("menu_button") ? ui_colors["menu_button"] : ImVec4(0,0,0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0f, 0.0f));
}

void xs::inspector::push_status_theme(theme t)
{
    // Status bar buttons use a transparent style by default
    ImGui::PushStyleColor(ImGuiCol_Button, ui_colors.count("status_button") ? ui_colors["status_button"] : ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ui_colors.count("status_button_hover") ? ui_colors["status_button_hover"] : ImVec4(0,0,0,0.08f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui_colors.count("status_button_active") ? ui_colors["status_button_active"] : ImVec4(0,0,0,0.12f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));
}
