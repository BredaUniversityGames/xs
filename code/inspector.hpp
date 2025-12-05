#pragma once
#include <string>

namespace xs::inspector
{
	void initialize();
	void shutdown();
	void render(double dt);
	bool paused();
	bool should_restart();

	enum class notification_type { info, success, warning, error };
	int notify(notification_type type, const std::string& message, float time);
	void clear_notification(int id);

	// Get mouse X position in game viewport coordinates (virtual resolution)
	// Returns 0.0 if mouse is outside game viewport, clamped to game bounds
	float get_game_mouse_x();

	// Get mouse Y position in game viewport coordinates (virtual resolution)
	// Returns 0.0 if mouse is outside game viewport, clamped to game bounds
	float get_game_mouse_y();
}
