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
	
	bool get_mouse();
	bool get_mousebutton(int button);
	bool get_mousebutton_once(int button);
	double get_mouse_x();
	double get_mouse_y();

	int get_nr_touches();
	int get_touch_id(int index);
	double get_touch_x(int index);
	double get_touch_y(int index);

	/// <summary>
	/// An enum listing all possible gamepad buttons with digital input values.
	/// This is the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further mapping.
	/// </summary>
	enum gamepad_button
	{
		BUTTON_SOUTH = 0,
		BUTTON_EAST = 1,
		BUTTON_WEST = 2,
		BUTTON_NORTH = 3,

		SHOULDER_LEFT = 4,
		SHOULDER_RIGHT = 5,

		BUTTON_SELECT = 6,
		BUTTON_START = 7,

		// Button 8 is not used

		STICK_LEFT = 9,
		STICK_RIGHT = 10,

		DPAD_UP = 11,
		DPAD_RIGHT = 12,
		DPAD_DOWN = 13,
		DPAD_LEFT = 14
	};

	/// <summary>
	/// An enum listing all possible gamepad axes with analog input values.
	/// This is the same numbering as in GLFW input, so a GLFW-based implementation can use it directly without any further mapping.
	/// </summary>
	enum gamepad_axis
	{
		/// Represents the horizontal axis of the left gamepad stick, with an analog input value between -1 (left) and 1 (right).
		STICK_LEFT_X = 0,
		/// Represents the vertical axis of the left gamepad stick, with an analog input value between -1 (down) and 1 (up).
		STICK_LEFT_Y = 1,
		/// Represents the horizontal axis of the right gamepad stick, with an analog input value between -1 (left) and 1 (right).
		STICK_RIGHT_X = 2,
		/// Represents the vertical axis of the right gamepad stick, with an analog input value between -1 (down) and 1 (up).
		STICK_RIGHT_Y = 3,
		/// Represents the left trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
		TRIGGER_LEFT = 4,
		/// Represents the right trigger of a gamepad, with an analog input value between 0 (not pressed) and 1 (fully pressed).
		TRIGGER_RIGHT = 5
	};
}
