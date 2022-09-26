#pragma once

namespace xs::input
{
	void initialize();
	void shutdown();
	void update(double dt);

	/// <summary>
	/// Returns the current floating-point value of a given gamepad axis.
	/// </summary>
	/// <param name="button">The ID of the axis to check; should be a value of the xs::input::gamepad_axis enum.</param>
	/// <returns>The current value of the given axis. For the possible ranges per axis, see the documentation of xs::input::gamepad_axis.</returns>
	double get_axis(int axis);

	/// <summary>
	/// Checks and returns whether a given gamepad button is currently being held down.
	/// </summary>
	/// <param name="button">The ID of the button to check; should be a value of the xs::input::gamepad_button enum.</param>
	/// <returns>true if the given button is being held down; false otherwise.</returns>
	bool get_button(int button);

	/// <summary>
	/// Checks and returns whether a given gamepad button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The ID of the button to check; should be a value of the xs::input::gamepad_button enum.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	bool get_button_once(int button);

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
	bool get_mousebutton(int button);

	/// <summary>
	/// Checks and returns whether a given mouse button is being pressed in the current frame without having been pressed in the previous frame.
	/// </summary>
	/// <param name="button">The keycode of the mouse button to check.</param>
	/// <returns>true if the given button is being pressed in this frame and not in the previous; false otherwise.</returns>
	bool get_mousebutton_once(int button);

	/// <summary>
	/// Gets the current X coordinate of the mouse position in game coordinates.
	/// </summary>
	double get_mouse_x();
	
	/// <summary>
	/// Gets the current Y coordinate of the mouse position in game coordinates.
	/// </summary>
	double get_mouse_y();

	/// <summary>
	/// Gets the current numbers of point where the touchscreen is being touched.
	/// </summary>
	int get_nr_touches();

	/// <summary>
	/// Gets the unique ID corresponding to a touch array index.
	/// This can be used to track touches of the same finger across multiple frames.
	/// The array index may change per frame, but the ID will not.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	int get_touch_id(int index);

	/// <summary>
	/// Gets the current X coordinate (in game coordinates) of the touch position at thet given array index.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
	double get_touch_x(int index);

	/// <summary>
	/// Gets the current Y coordinate (in game coordinates) of the touch position at thet given array index.
	/// </summary>
	/// <param name="index">The array index of the touch point to check.</param>
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
