#include "input.hpp"
#include <cassert>
#include <array>
#include <unordered_map>

#include <isteaminput.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

#include "device.hpp"
#include "device_pc.hpp"
#include "configuration.hpp"
#include "log.hpp"

/*
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

	double mouse_scroll = 0.0;

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

	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		mouse_scroll += yoffset;
	}
}

void xs::input::initialize()
{
	glfwSetJoystickCallback(joystick_callback);
	glfwSetCursorPosCallback(device::get_window(), cursor_position_callback);
	glfwSetKeyCallback(device::get_window(), key_callback);
	glfwSetMouseButtonCallback(device::get_window(), mousebutton_callback);
	glfwSetScrollCallback(device::get_window(), scroll_callback);

	update(0.0f);

	if (SteamInput() != nullptr)
		SteamInput()->Init(false);
}

void xs::input::shutdown()
{
	glfwSetJoystickCallback(NULL);
	glfwSetCursorPosCallback(device::get_window(), NULL);

	if (SteamInput() != nullptr)
		SteamInput()->Shutdown();
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

	if (SteamInput() != nullptr)
	{
		std::array<InputHandle_t, STEAM_INPUT_MAX_COUNT> inputHandles{};
		int connectedControllers = SteamInput()->GetConnectedControllers(&inputHandles[0]);
		if (connectedControllers > 0)
		{
			int g = 0;
		}
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

double xs::input::get_mouse_wheel()
{
	return mouse_scroll;
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
*/

// SDL implementation

using namespace xs;
using namespace std;

namespace xs::input
{

struct gamepad_data
{
	bool buttons[SDL_GAMEPAD_BUTTON_COUNT];
	float axes[SDL_GAMEPAD_AXIS_COUNT];

	gamepad_data() { memset(this, 0, sizeof(*this)); }

	// Copy the previous gamepad state
	gamepad_data operator=(const gamepad_data& other)
	{
		memcpy(buttons, other.buttons, sizeof(buttons));
		memcpy(axes, other.axes, sizeof(axes));
		return *this;
	}
};


struct gamepad_entry
{
	gamepad_data gamepad;
	gamepad_data prev_gamepad;
	SDL_Gamepad* sdl_pad;

	gamepad_entry() : sdl_pad(nullptr) {}
	gamepad_entry(SDL_Gamepad* pad) : sdl_pad(pad) {}
};

struct Data
{
	unordered_map<size_t, gamepad_entry> gamepads;
};

gamepad_entry* get_default_gamepad();
void update_all_gamepads();
Data* data = nullptr;

}

// Dummy implementation for the PC platform (for now)
void xs::input::initialize()
{
	data = new Data();
	SDL_Init(SDL_INIT_GAMEPAD);
}
void xs::input::shutdown()
{	
	for (auto& j : data->gamepads)
		SDL_CloseGamepad(j.second.sdl_pad);
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	delete data;
}

void xs::input::update(double dt)
{
	update_all_gamepads();
}

double xs::input::get_mouse_x() { return 0.0; }
double xs::input::get_mouse_y() { return 0.0; }
double xs::input::get_mouse_wheel() { return 0.0; }

int xs::input::get_nr_touches() { return 0; }
int xs::input::get_touch_id(int index) { return 0; }
double xs::input::get_touch_x(int index) { return 0.0; }
double xs::input::get_touch_y(int index) { return 0.0; }

void xs::input::set_gamepad_vibration(int leftRumble, int rightRumble) {}
void xs::input::set_lightbar_color(double red, double green, double blue) {}
void xs::input::reset_lightbar() {}

bool xs::input::get_key(int key) { return false; }
bool xs::input::get_key_once(int key) { return false; }

bool xs::input::get_mouse()
{
	return false;
}

bool xs::input::get_mousebutton(mouse_button button)
{
	// Get mouse button with SDL3
	float x, y;
	Uint32 mouseState = SDL_GetMouseState(&x, &y);
	return (mouseState & SDL_BUTTON_MASK(button)) != 0;

}
bool xs::input::get_mousebutton_once(mouse_button button) { return false; }

bool xs::input::get_button(gamepad_button button)
{
	auto gamepad = get_default_gamepad();
	if (gamepad == nullptr)
		return false;

	auto value = gamepad->gamepad.buttons[button];
	return value == 1;
}

bool xs::input::get_button_once(gamepad_button button)
{
	auto gamepad = get_default_gamepad();
	if (gamepad == nullptr)
		return false;
	auto value = gamepad->gamepad.buttons[button];
	auto value_prev = gamepad->prev_gamepad.buttons[button];
	return value == 1 && value_prev == 0;
}

double xs::input::get_axis(gamepad_axis axis)
{
	auto gamepad = get_default_gamepad();
	if (gamepad == nullptr)
		return 0.0;
	auto value = gamepad->gamepad.axes[axis];
	return value;
}

xs::input::gamepad_entry * xs::input::get_default_gamepad()
{
	if (data->gamepads.size() > 0)
		return &data->gamepads[0];
	return nullptr;
}

void xs::input::update_all_gamepads()
{
	int count = 0;
	auto* joysticks = SDL_GetGamepads(&count);
	for (int i = 0; i < count; i++)
	{
		auto player = SDL_GetGamepadPlayerIndexForID(joysticks[i]);

		// Check if the gamepad is already open
		if (data->gamepads.find(player) == data->gamepads.end())
		{
			auto gamepad = SDL_OpenGamepad(joysticks[i]);
			if (gamepad == nullptr)
				xs::log::error("Failed to open gamepad {}", i);
			else
				data->gamepads[player] = gamepad_entry(gamepad); // Init to zero
		}

		// The gamepad is already open or just added		
		if (data->gamepads.find(player) != data->gamepads.end())
		{
			// Copy the previous gamepad state
			data->gamepads[player].prev_gamepad = data->gamepads[player].gamepad;

			// Get the current gamepad state
			auto sdl_pad = data->gamepads[player].sdl_pad;

			for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
				data->gamepads[player].gamepad.buttons[i] = SDL_GetGamepadButton(sdl_pad, (SDL_GamepadButton)i);
			for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++)
				data->gamepads[player].gamepad.axes[i] = SDL_GetGamepadAxis(sdl_pad, (SDL_GamepadAxis)i) / 32767.0f;			
		}		
	}
}
