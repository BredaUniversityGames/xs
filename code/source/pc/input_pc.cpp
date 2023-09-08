#include "input.h"
#include <cassert>
#include <array>
#include <GLFW/glfw3.h>

#include <isteaminput.h>

#include "device.h"
#include "device_pc.h"
#include "configuration.h"

namespace 
{
	enum KeyAction { Release = 0, Press = 1, None = 2 };

	GLFWgamepadstate gamepad_state;
	GLFWgamepadstate prev_gamepad_state;

	constexpr int nr_keys = 350;
	bool keys_down[nr_keys];
	bool prev_keys_down[nr_keys];
	KeyAction keys_action[nr_keys];
	
	constexpr int nr_mousebuttons = 8;
	bool mousebuttons_down[nr_mousebuttons];
	bool prev_mousebuttons_down[nr_mousebuttons];
	bool mousebuttons_down_changed[nr_mousebuttons];
	KeyAction mousebuttons_action[nr_mousebuttons];

	float mousepos[2];
	bool gamepad_connected;

	void joystick_callback(int joy, int event) {}
	void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
	{
		// get the screen-to-game scaling parameters
		const auto& screen_to_game = xs::configuration::get_scale_to_game(xs::device::get_width(), xs::device::get_height());

		// translate the mouse position to game coordinates
		xs::configuration::scale_to_game(static_cast<int>(xpos), static_cast<int>(ypos), screen_to_game, mousepos[0], mousepos[1]);
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS || action == GLFW_RELEASE)
			keys_action[key] = static_cast<KeyAction>(action);
	}

	void mousebutton_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (action == GLFW_PRESS || action == GLFW_RELEASE)
			mousebuttons_action[button] = static_cast<KeyAction>(action);
	}
}

void xs::input::initialize()
{
	glfwSetJoystickCallback(joystick_callback);
	glfwSetCursorPosCallback(device::get_window(), cursor_position_callback);
	glfwSetKeyCallback(device::get_window(), key_callback);
	glfwSetMouseButtonCallback(device::get_window(), mousebutton_callback);

	update(0.0f);

	bool success = SteamInput()->Init(false);
}

void xs::input::shutdown()
{
	glfwSetJoystickCallback(NULL);
	glfwSetCursorPosCallback(device::get_window(), NULL);

	bool success = SteamInput()->Shutdown();
}

void xs::input::update(double dt)
{
	for (int i = 0; i < nr_keys; ++i)
	{
		prev_keys_down[i] = keys_down[i];

		if (keys_action[i] == KeyAction::Press)
			keys_down[i] = true;
		else if (keys_action[i] == KeyAction::Release)
			keys_down[i] = false;

		keys_action[i] = KeyAction::None;
	}

	for (int i = 0; i < nr_mousebuttons; ++i)
	{
		prev_mousebuttons_down[i] = mousebuttons_down[i];

		if (mousebuttons_action[i] == KeyAction::Press)
			mousebuttons_down[i] = true;
		else if (mousebuttons_action[i] == KeyAction::Release)
			mousebuttons_down[i] = false;

		mousebuttons_action[i] = KeyAction::None;
	}

	prev_gamepad_state = gamepad_state;


	std::array<InputHandle_t, STEAM_INPUT_MAX_COUNT> inputHandles{};
	int connectedControllers = SteamInput()->GetConnectedControllers(&inputHandles[0]);
	if (connectedControllers > 0)
	{
		int g = 0;
	}

	if (glfwJoystickPresent(0) && glfwJoystickIsGamepad(0))
		gamepad_connected = glfwGetGamepadState(0, &gamepad_state);
}

double xs::input::get_axis(gamepad_axis axis)
{
	if (!gamepad_connected) return 0.0;
	
	assert(axis >= 0 && axis <= GLFW_GAMEPAD_AXIS_LAST);
	return static_cast<double>(gamepad_state.axes[axis]);
}

bool xs::input::get_button(gamepad_button button)
{
	if (!gamepad_connected) return false;
	
	assert(button >= 0 && button <= GLFW_GAMEPAD_BUTTON_LAST);
	return static_cast<bool>(gamepad_state.buttons[button]);
}

bool xs::input::get_button_once(gamepad_button button)
{
	if (!gamepad_connected) return false;
	
	assert(button >= 0 && button <= GLFW_GAMEPAD_BUTTON_LAST);
	return
		!static_cast<bool>(prev_gamepad_state.buttons[button]) &&
		static_cast<bool>(gamepad_state.buttons[button]);
}

bool xs::input::get_key(int key)
{
	assert(key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST);
	return keys_down[key];
}

bool xs::input::get_key_once(int key)
{
	assert(key >= GLFW_KEY_SPACE && key <= GLFW_KEY_LAST);
	return keys_down[key] && !prev_keys_down[key];
}

bool xs::input::get_mouse()
{
	return true;
}

bool xs::input::get_mousebutton(mouse_button button)
{
	return mousebuttons_down[button];
}

bool xs::input::get_mousebutton_once(mouse_button button)
{
	return mousebuttons_down[button] && !prev_mousebuttons_down[button];
}

double xs::input::get_mouse_x()
{
	return static_cast<double>(mousepos[0]);
}

double xs::input::get_mouse_y()
{
	return static_cast<double>(mousepos[1]);
}

int xs::input::get_nr_touches()
{
	return 0;
}

int xs::input::get_touch_id(int index)
{
	return 0;
}

double xs::input::get_touch_x(int index)
{
	return 0.0;
}

double xs::input::get_touch_y(int index)
{
	return 0.0;
}

void xs::input::set_gamepad_vibration(int smallRumble, int largeRumble)
{
	//Unimplemented on the PC
}

void xs::input::set_lightbar_color(double red, double green, double blue)
{
	//Unimplemented on the PC (specific dualshock 5 controller mechanic)
}

void xs::input::reset_lightbar()
{
	//Unimplemented on the PC (specific dualshock 5 controller mechanic)
}
