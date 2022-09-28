#include "input.h"
#include <cassert>
#include <GLFW/glfw3.h>

#include "device.h"
#include "device_pc.h"
#include "configuration.h"

namespace 
{
	GLFWgamepadstate gamepad_state;
	GLFWgamepadstate prev_gamepad_state;
	char key_once[512 + 1];
	char mousebutton_once[8];
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

}

void xs::input::initialize()
{
	glfwSetJoystickCallback(joystick_callback);
	glfwSetCursorPosCallback(device::get_window(), cursor_position_callback);
	update(0.0f);
}

void xs::input::shutdown()
{
	glfwSetJoystickCallback(NULL);
	glfwSetCursorPosCallback(device::get_window(), NULL);
}

void xs::input::update(double dt)
{
	prev_gamepad_state = gamepad_state;
	if (glfwJoystickPresent(0) && glfwJoystickIsGamepad(0))
		gamepad_connected = glfwGetGamepadState(0, &gamepad_state);
}

double xs::input::get_axis(gamepad_axis axis)
{
	if (!gamepad_connected) return 0.0;
	
	assert(axis >= 0 && axis <= GLFW_GAMEPAD_AXIS_LAST);
	//return (static_cast<double>(gamepad_state.axes[axis]) + 1.0) * 0.5;
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
	return glfwGetKey(device::get_window(), key) == GLFW_PRESS;
}

bool xs::input::get_key_once(int key)
{
	/*
	auto k = key_once[key];
	return glfwGetKey(device::get_window(), key) == GLFW_PRESS;
	*/

	return (glfwGetKey(device::get_window(), key) ?
		(key_once[key] ? false : (key_once[key] = true)) : \
		(key_once[key] = false));
}

bool xs::input::get_mouse()
{
	return true;
}

bool xs::input::get_mousebutton(mouse_button button)
{
	return glfwGetMouseButton(device::get_window(), button) == GLFW_PRESS;
}

bool xs::input::get_mousebutton_once(mouse_button button)
{
	return (glfwGetMouseButton(device::get_window(), button) ?
		(mousebutton_once[button] ? false : (mousebutton_once[button] = true)) : \
		(mousebutton_once[button] = false));
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