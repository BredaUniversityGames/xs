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

    enum class theme { light, dark };
    theme get_theme();

    struct frame { float top_bar = 0.0f; float bottom_bar = 0.0f; };
    frame get_frame();
}
