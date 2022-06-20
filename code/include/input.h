#pragma once

namespace xs::input
{
	void initialize();
	void shutdown();
	void update(float dt);
	double get_axis(int axis);
	bool get_button(int button);
	bool get_button_once(int button);
	bool get_key(int key);
	bool get_key_once(int key);
}
