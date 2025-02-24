#pragma once

namespace xs::input
{
	void initialize();
	void shutdown();
	void update(double dt);

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

	enum mouse_button {

		MOUSE_BUTTON_LEFT = 0,
		MOUSE_BUTTON_RIGHT = 1,
		MOUSE_BUTTON_MIDDLE = 2
	};

	enum key_code {
		KEY_SPACE = 32,
		KEY_APOSTROPHE = 39,
		KEY_COMMA = 44,
		KEY_MINUS = 45,
		KEY_PERIOD = 46,
		KEY_SLASH = 47,
		KEY_0 = 48,
		KEY_1 = 49,
		KEY_2 = 50,
		KEY_3 = 51,
		KEY_4 = 52,
		KEY_5 = 53,
		KEY_6 = 54,
		KEY_7 = 55,
		KEY_8 = 56,
		KEY_9 = 57,
		KEY_SEMICOLON = 59,
		KEY_EQUAL = 61,
		KEY_A = 65,
		KEY_B = 66,
		KEY_C = 67,
		KEY_D = 68,
		KEY_E = 69,
		KEY_F = 70,
		KEY_G = 71,
		KEY_H = 72,
		KEY_I = 73,
		KEY_J = 74,
		KEY_K = 75,
		KEY_L = 76,
		KEY_M = 77,
		KEY_N = 78,
		KEY_O = 79,
		KEY_P = 80,
		KEY_Q = 81,
		KEY_R = 82,
		KEY_S = 83,
		KEY_T = 84,
		KEY_U = 85,
		KEY_V = 86,
		KEY_W = 87,
		KEY_X = 88,
		KEY_Y = 89,
		KEY_Z = 90,
		KEY_LEFT_BRACKET = 91,
		KEY_BACKSLASH = 92,
		KEY_RIGHT_BRACKET = 93,
		KEY_GRAVE_ACCENT = 96,
		KEY_WORLD_1 = 161,
		KEY_WORLD_2 = 162,
		KEY_ESCAPE = 256,
		KEY_ENTER = 257,
		KEY_TAB = 258,
		KEY_BACKSPACE = 259,
		KEY_INSERT = 260,
		KEY_DELETE = 261,
		KEY_RIGHT = 262,
		KEY_LEFT = 263,
		KEY_DOWN = 264,
		KEY_UP = 265,
		KEY_PAGE_UP = 266,
		KEY_PAGE_DOWN = 267,
		KEY_HOME = 268,
		KEY_END = 269,
		KEY_CAPS_LOCK = 280,
		KEY_SCROLL_LOCK = 281,
		KEY_NUM_LOCK = 282,
		KEY_PRINT_SCREEN = 283,
		KEY_PAUSE = 284,
		KEY_F1 = 290,
		KEY_F2 = 291,
		KEY_F3 = 292,
		KEY_F4 = 293,
		KEY_F5 = 294,
		KEY_F6 = 295,
		KEY_F7 = 296,
		KEY_F8 = 297,
		KEY_F9 = 298,
		KEY_F10 = 299,
		KEY_F11 = 300,
		KEY_F12 = 301		
	};

	/*
	/// <summary>
	/// Returns the current floating-point value of a given gamepad axis.
	/// </summary>
	/// <param name="button">The ID of the axis to check.</param>
	/// <returns>The current value of the given axis. For the possible ranges per axis, see the documentation of xs::input::gamepad_axis.</returns>
	double get_axis(int pad, gamepad_axis axis);

	/// <summary>
	/// Checks and returns whether a given gamepad button is currently being held down.
	/// </summary>
	/// <param name="button">The ID of the button to check.</param>
	/// <returns>true if the given button is being held down; false otherwise.</returns>
	bool get_button(int pad, gamepad_button button);

	/// <summary>
	/// Checks and returns whether a given gamepad button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The ID of the button to check.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	bool get_button_once(int pad, gamepad_button button);
	*/

	/// <summary>
	/// Returns the current floating-point value of a given gamepad axis.
	/// </summary>
	/// <param name="axis">The ID of the axis to check.</param>
	/// <returns>The current value of the given axis. For the possible ranges per axis, see the documentation of xs::input::gamepad_axis.</returns>
	double get_axis(gamepad_axis axis);


	/// <summary>
	/// Returns true if the given axis is currently being held down and its value is greater than the given threshold.
	/// </summary>
	/// <param name="axis">The ID of the axis to check.</param>
	/// <param name="threshold">The threshold value to compare the axis value against.</param> 
	bool get_axis_once(gamepad_axis axis, double threshold);
	
	/// <summary>
	/// Checks and returns whether a given gamepad button is currently being held down.
	/// </summary>
	/// <param name="button">The ID of the button to check.</param>
	/// <returns>true if the given button is being held down; false otherwise.</returns>
	bool get_button(gamepad_button button);

	/// <summary>
	/// Checks and returns whether a given gamepad button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The ID of the button to check.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	bool get_button_once(gamepad_button button);

	/// <summary>
	/// Checks and returns whether a given keyboard key is currently being held down.
	/// </summary>
	/// <param name="key">The keycode of the key to check.</param>
	/// <returns>true if the given key is being held down; false otherwise.</returns>
	bool get_key(int key);

	/// <summary>
	/// Checks and returns whether a given keyboard key is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="key">The keycode of the key to check.</param>
	/// <returns>true if the given key is being pressed in this frame and not in the previous; false otherwise.</returns>
	bool get_key_once(int key);
	
	/// <summary>
	/// Checks and returns whether mouse input is currently available.
	/// </summary>
	bool get_mouse();

	/// <summary>
	/// Checks and returns whether a given mouse button is currently being held down.
	/// </summary>
	/// <param name="button">The keycode of the mouse button to check.</param>
	/// <returns>true if the given button is being held down; false otherwise.</returns>
	bool get_mousebutton(mouse_button button);

	/// <summary>
	/// Checks and returns whether a given mouse button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The keycode of the mouse button to check.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	bool get_mousebutton_once(mouse_button button);

	/// <summary>
	/// Gets the current X coordinate of the mouse position in game coordinates.
	/// </summary>
	double get_mouse_x();
	
	/// <summary>
	/// Gets the current Y coordinate of the mouse position in game coordinates.
	/// </summary>
	double get_mouse_y();

	/// <summary>
	/// Gets the mouse wheel, relative to the initial value when starting the game.
	/// </summary>
	double get_mouse_wheel();

	/// <summary>
	/// Gets the current numbers of point where the touchscreen is being touched.
	/// </summary>
	int get_nr_touches();

	/// <summary>
	/// Gets the unique ID corresponding to a touch array index, or -1 if that touch does not exist.
	/// This can be used to track touches of the same finger across multiple frames.
	/// The array index may change per frame, but the ID will not.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	int get_touch_id(int index);

	/// <summary>
	/// Gets the current X coordinate (in game coordinates) of the touch position at the given array index.
	/// Returns 0.0 if the given touch does not exist.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	double get_touch_x(int index);

	/// <summary>
	/// Gets the current Y coordinate (in game coordinates) of the touch position at the given array index.
	/// Returns 0.0 if the given touch does not exist.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	double get_touch_y(int index);

	/// <summary>
	/// Turns on vibration for the left and right motor if the used controller supports this
	/// </summary>
	/// <param name="low">Value between 0 (off) and 1 (max rotation) for the left motor</param>
	/// <param name="`high">Value between 0 (off) and 1 (max rotation) for the right motor</param>
	void set_gamepad_vibration(double low, double high, double time);

	/// <summary>
	/// Sets the color value for the lightbar on Dualshock controllers (4 and up)
	/// <para> Currently only implemented for the PS5 and dualshock 4+ controllers</para>
	/// </summary>
	/// <param name="red, green, blue"> The new coulour values in RGB format ranging between 0 and 255</param>
	void set_lightbar_color(double red, double green, double blue);

	/// <summary>
	/// Turns off the light bar
	/// <para> Currently only implemented for the PS5 and dualshock 4+ controllers</para> 
	/// </summary>
	void reset_lightbar();
}
