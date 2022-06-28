#pragma once

namespace xs::input
{
	void initialize();
	void shutdown();
	void update(double dt);
	double get_axis(int axis);
	bool get_button(int button);
	bool get_button_once(int button);
	bool get_key(int key);
	bool get_key_once(int key);
	bool get_mousebutton(int button);
	bool get_mousebutton_once(int button);
	double get_mouse_x();
	double get_mouse_y();
}
