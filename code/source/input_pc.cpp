#include "input.h"
#include <cassert>
#include <GLFW/glfw3.h>

#include "device_pc.h"

namespace 
{
	void joystick_callback(int joy, int event) {}
	GLFWgamepadstate gamepad_state;
	GLFWgamepadstate prev_gamepad_state;
	char key_once[256 + 1];
	bool gamepad_connected;
}

void xs::input::initialize()
{
	glfwSetJoystickCallback(joystick_callback);
	update();
}

void xs::input::shutdown()
{
	glfwSetJoystickCallback(NULL);
}

void xs::input::update()
{
	prev_gamepad_state = gamepad_state;
	if (glfwJoystickPresent(0) && glfwJoystickIsGamepad(0))
		gamepad_connected = glfwGetGamepadState(0, &gamepad_state);
}

double xs::input::get_axis(int axis)
{
	if (!gamepad_connected) return 0.0;
	
	assert(axis >= 0 && axis < GLFW_GAMEPAD_AXIS_LAST);
	//return (static_cast<double>(gamepad_state.axes[axis]) + 1.0) * 0.5;
	return static_cast<double>(gamepad_state.axes[axis]);
}

bool xs::input::get_button(int button)
{
	if (!gamepad_connected) return false;
	
	assert(button >= 0 && button < GLFW_GAMEPAD_BUTTON_LAST);
	return static_cast<bool>(gamepad_state.buttons[button]);
}

bool xs::input::get_button_once(int button)
{
	if (!gamepad_connected) return false;
	
	assert(button >= 0 && button < GLFW_GAMEPAD_BUTTON_LAST);
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
	return (glfwGetKey(device::get_window(), key) ?
		(key_once[key] ? false : (key_once[key] = true)) : \
		(key_once[key] = false));
}
