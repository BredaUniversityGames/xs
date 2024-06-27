#pragma once
#include <string>

namespace xs::inspector
{	
	void initialize();
	void shutdown();
	void render(float dt);
	bool paused();

	enum class notification_type { info, success, warning, error };
	int notify(notification_type type, const std::string& message, float time);
	void clear_notification(int id);
}
