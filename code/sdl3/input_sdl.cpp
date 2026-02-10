#include "input.hpp"
#include <cassert>
#include <array>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

#include "device.hpp"
#include "device_sdl.hpp"
#include "configuration.hpp"
#include "log.hpp"
#include "data.hpp"


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

	bool keys_down[SDL_SCANCODE_COUNT];
	bool prev_keys_down[SDL_SCANCODE_COUNT];

	// Mouse state
	float mouse_x = 0.0f;
	float mouse_y = 0.0f;
	float mouse_wheel = 0.0f;
	bool mouse_buttons[5] = {false}; // Left, Middle, Right, X1, X2
	bool prev_mouse_buttons[5] = {false};
};

gamepad_entry* get_default_gamepad();
void update_all_gamepads();
void update_keyboard();
void update_mouse();
int get_usb_scancode(int key);
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
	update_keyboard();
	update_mouse();
}

double xs::input::get_mouse_x()
{
	return data ? data->mouse_x : 0.0;
}

double xs::input::get_mouse_y()
{
	return data ? data->mouse_y : 0.0;
}

double xs::input::get_mouse_wheel()
{
	return data ? data->mouse_wheel : 0.0;
}

int xs::input::get_nr_touches() { return 0; }
int xs::input::get_touch_id(int index) { return 0; }
double xs::input::get_touch_x(int index) { return 0.0; }
double xs::input::get_touch_y(int index) { return 0.0; }

void xs::input::set_gamepad_vibration(double low, double high, double time)
{
#ifdef XS_DEBUG
	if (data::get_bool("Vibration.Disabled", data::type::debug))
		return;
#endif
	
	auto gamepad = get_default_gamepad();
	if (gamepad == nullptr)
		return;

	uint16_t low_rumble = (uint16_t)(low * 0xFFFF);
	uint16_t high_rumble = (uint16_t)(high * 0xFFFF);
	SDL_RumbleGamepad(gamepad->sdl_pad, high_rumble, low_rumble, (uint32_t)(time * 1000));
}

void xs::input::set_lightbar_color(double red, double green, double blue) {}
void xs::input::reset_lightbar() {}

bool xs::input::get_key(int key)
{
	auto scanout = get_usb_scancode(key);
	if(0 <= scanout && scanout < SDL_SCANCODE_COUNT)
		return data->keys_down[scanout];
	return false;
}

bool xs::input::get_key_once(int key)
{
	auto scanout = get_usb_scancode(key);
	if (0 <= scanout && scanout < SDL_SCANCODE_COUNT)
		return data->keys_down[scanout] && !data->prev_keys_down[scanout];
	return false;
}

bool xs::input::get_mouse()
{
	return true;
}

bool xs::input::get_mousebutton(mouse_button button)
{
	if (!data || button < 0 || button >= 5)
		return false;
	return data->mouse_buttons[button];
}

bool xs::input::get_mousebutton_once(mouse_button button)
{
	if (!data || button < 0 || button >= 5)
		return false;
	return data->mouse_buttons[button] && !data->prev_mouse_buttons[button];
}

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

bool input::get_axis_once(gamepad_axis axis, double threshold)
{
	auto gamepad = get_default_gamepad();
	if (gamepad == nullptr)
		return false;

	bool value = threshold < 0.0 ?
		gamepad->gamepad.axes[axis] < threshold && gamepad->prev_gamepad.axes[axis] >= threshold :
		gamepad->gamepad.axes[axis] > threshold && gamepad->prev_gamepad.axes[axis] <= threshold;
	return value;
}

xs::input::gamepad_entry* xs::input::get_default_gamepad()
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

void xs::input::update_keyboard()
{
	int count = 0;
	const bool* keys = SDL_GetKeyboardState(&count);
	count = min(count, (int)SDL_SCANCODE_COUNT);

	auto size = count * sizeof(bool);

	memcpy(data->prev_keys_down, data->keys_down, size);
	memcpy(data->keys_down, keys, size);
}

void xs::input::update_mouse()
{
	// Save previous mouse button state
	memcpy(data->prev_mouse_buttons, data->mouse_buttons, sizeof(data->mouse_buttons));

	// Get current mouse state
	Uint32 mouse_state = SDL_GetMouseState(&data->mouse_x, &data->mouse_y);

	// Update button states (SDL uses 1-based button indices)
	data->mouse_buttons[0] = (mouse_state & SDL_BUTTON_LMASK) != 0; // Left
	data->mouse_buttons[1] = (mouse_state & SDL_BUTTON_MMASK) != 0; // Middle
	data->mouse_buttons[2] = (mouse_state & SDL_BUTTON_RMASK) != 0; // Right
	data->mouse_buttons[3] = (mouse_state & SDL_BUTTON_X1MASK) != 0; // X1
	data->mouse_buttons[4] = (mouse_state & SDL_BUTTON_X2MASK) != 0; // X2

	// Mouse wheel is reset each frame (it's handled via events)
	data->mouse_wheel = 0.0f;
}

int xs::input::get_usb_scancode(int key)
{
	int scanout = -1;
	switch (key)
	{
	case KEY_SPACE: scanout = SDL_SCANCODE_SPACE; break;
	case KEY_APOSTROPHE: scanout = SDL_SCANCODE_APOSTROPHE; break;
	case KEY_COMMA: scanout = SDL_SCANCODE_COMMA; break;
	case KEY_MINUS: scanout = SDL_SCANCODE_MINUS; break;
	case KEY_PERIOD: scanout = SDL_SCANCODE_PERIOD; break;
	case KEY_SLASH: scanout = SDL_SCANCODE_SLASH; break;
	case KEY_0: scanout = SDL_SCANCODE_0; break;
	case KEY_1: scanout = SDL_SCANCODE_1; break;
	case KEY_2: scanout = SDL_SCANCODE_2; break;
	case KEY_3: scanout = SDL_SCANCODE_3; break;
	case KEY_4: scanout = SDL_SCANCODE_4; break;
	case KEY_5: scanout = SDL_SCANCODE_5; break;
	case KEY_6: scanout = SDL_SCANCODE_6; break;
	case KEY_7: scanout = SDL_SCANCODE_7; break;
	case KEY_8: scanout = SDL_SCANCODE_8; break;
	case KEY_9: scanout = SDL_SCANCODE_9; break;
	case KEY_SEMICOLON: scanout = SDL_SCANCODE_SEMICOLON; break;
	//case KEY_EQUALS: scanout = SDL_SCANCODE_EQUALS; break;
	case KEY_A: scanout = SDL_SCANCODE_A; break;
	case KEY_B: scanout = SDL_SCANCODE_B; break;
	case KEY_C: scanout = SDL_SCANCODE_C; break;
	case KEY_D: scanout = SDL_SCANCODE_D; break;
	case KEY_E: scanout = SDL_SCANCODE_E; break;
	case KEY_F: scanout = SDL_SCANCODE_F; break;
	case KEY_G: scanout = SDL_SCANCODE_G; break;
	case KEY_H: scanout = SDL_SCANCODE_H; break;
	case KEY_I: scanout = SDL_SCANCODE_I; break;
	case KEY_J: scanout = SDL_SCANCODE_J; break;
	case KEY_K: scanout = SDL_SCANCODE_K; break;
	case KEY_L: scanout = SDL_SCANCODE_L; break;
	case KEY_M: scanout = SDL_SCANCODE_M; break;
	case KEY_N: scanout = SDL_SCANCODE_N; break;
	case KEY_O: scanout = SDL_SCANCODE_O; break;
	case KEY_P: scanout = SDL_SCANCODE_P; break;
	case KEY_Q: scanout = SDL_SCANCODE_Q; break;
	case KEY_R: scanout = SDL_SCANCODE_R; break;
	case KEY_S: scanout = SDL_SCANCODE_S; break;
	case KEY_T: scanout = SDL_SCANCODE_T; break;
	case KEY_U: scanout = SDL_SCANCODE_U; break;
	case KEY_V: scanout = SDL_SCANCODE_V; break;
	case KEY_W: scanout = SDL_SCANCODE_W; break;
	case KEY_X: scanout = SDL_SCANCODE_X; break;
	case KEY_Y: scanout = SDL_SCANCODE_Y; break;
	case KEY_Z: scanout = SDL_SCANCODE_Z; break;
	//case KEY_LEFTBRACKET: scanout = SDL_SCANCODE_LEFTBRACKET; break;
	case KEY_BACKSLASH: scanout = SDL_SCANCODE_BACKSLASH; break;
	//case KEY_RIGHTBRACKET: scanout = SDL_SCANCODE_RIGHTBRACKET; break;
	//case KEY_GRAVE: scanout = SDL_SCANCODE_GRAVE; break;
	case KEY_ESCAPE: scanout = SDL_SCANCODE_ESCAPE; break;
	//case KEY_RETURN: scanout = SDL_SCANCODE_RETURN; break;
	case KEY_TAB: scanout = SDL_SCANCODE_TAB; break;
	case KEY_BACKSPACE: scanout = SDL_SCANCODE_BACKSPACE; break;
	case KEY_INSERT: scanout = SDL_SCANCODE_INSERT; break;
	case KEY_DELETE: scanout = SDL_SCANCODE_DELETE; break;
	case KEY_RIGHT: scanout = SDL_SCANCODE_RIGHT; break;
	case KEY_LEFT: scanout = SDL_SCANCODE_LEFT; break;
	case KEY_DOWN: scanout = SDL_SCANCODE_DOWN; break;
	case KEY_UP: scanout = SDL_SCANCODE_UP; break;
	//case KEY_PAGEUP: scanout = SDL_SCANCODE_PAGEUP; break;
	//case KEY_PAGEDOWN: scanout = SDL_SCANCODE_PAGEDOWN; break;
	case KEY_HOME: scanout = SDL_SCANCODE_HOME; break;
	case KEY_END: scanout = SDL_SCANCODE_END; break;
	//case KEY_CAPSLOCK: scanout = SDL_SCANCODE_CAPSLOCK; break;
	//case KEY_SCROLLLOCK: scanout = SDL_SCANCODE_SCROLLLOCK; break;
	//case KEY_NUMLOCKCLEAR: scanout = SDL_SCANCODE_NUMLOCKCLEAR; break;
	//case KEY_PRINTSCREEN: scanout = SDL_SCANCODE_PRINTSCREEN; break;
	case KEY_PAUSE: scanout = SDL_SCANCODE_PAUSE; break;
	case KEY_F1: scanout = SDL_SCANCODE_F1; break;
	case KEY_F2: scanout = SDL_SCANCODE_F2; break;
	case KEY_F3: scanout = SDL_SCANCODE_F3; break;
	case KEY_F4: scanout = SDL_SCANCODE_F4; break;
	case KEY_F5: scanout = SDL_SCANCODE_F5; break;
	case KEY_F6: scanout = SDL_SCANCODE_F6; break;
	case KEY_F7: scanout = SDL_SCANCODE_F7; break;
	case KEY_F8: scanout = SDL_SCANCODE_F8; break;
	case KEY_F9: scanout = SDL_SCANCODE_F9; break;
	case KEY_F10: scanout = SDL_SCANCODE_F10; break;
	case KEY_F11: scanout = SDL_SCANCODE_F11; break;
	case KEY_F12: scanout = SDL_SCANCODE_F12; break;
		/*
	case KEY_F13: scanout = SDL_SCANCODE_F13; break;
	case KEY_F14: scanout = SDL_SCANCODE_F14; break;
	case KEY_F15: scanout = SDL_SCANCODE_F15; break;
	case KEY_F16: scanout = SDL_SCANCODE_F16; break;
	case KEY_F17: scanout = SDL_SCANCODE_F17; break;
	case KEY_F18: scanout = SDL_SCANCODE_F18; break;
	case KEY_F19: scanout = SDL_SCANCODE_F19; break;
	case KEY_F20: scanout = SDL_SCANCODE_F20; break;
	case KEY_F21: scanout = SDL_SCANCODE_F21; break;
	case KEY_F22: scanout = SDL_SCANCODE_F22; break;
	case KEY_F23: scanout = SDL_SCANCODE_F23; break;
	case KEY_F24: scanout = SDL_SCANCODE_F24; break;
	case KEY_KP_0: scanout = SDL_SCANCODE_KP_0; break;
	case KEY_KP_1: scanout = SDL_SCANCODE_KP_1; break;
	case KEY_KP_2: scanout = SDL_SCANCODE_KP_2; break;
	case KEY_KP_3: scanout = SDL_SCANCODE_KP_3; break;
	case KEY_KP_4: scanout = SDL_SCANCODE_KP_4; break;
	case KEY_KP_5: scanout = SDL_SCANCODE_KP_5; break;
	case KEY_KP_6: scanout = SDL_SCANCODE_KP_6; break;
	case KEY_KP_7: scanout = SDL_SCANCODE_KP_7; break;
	case KEY_KP_8: scanout = SDL_SCANCODE_KP_8; break;
	case KEY_KP_9: scanout = SDL_SCANCODE_KP_9; break;
	case KEY_KP_PERIOD: scanout = SDL_SCANCODE_KP_PERIOD; break;
	case KEY_KP_DIVIDE: scanout = SDL_SCANCODE_KP_DIVIDE; break;
	case KEY_KP_MULTIPLY: scanout = SDL_SCANCODE_KP_MULTIPLY; break;
	case KEY_KP_MINUS: scanout = SDL_SCANCODE_KP_MINUS; break;
	case KEY_KP_PLUS: scanout = SDL_SCANCODE_KP_PLUS; break;
	case KEY_KP_ENTER: scanout = SDL_SCANCODE_KP_ENTER; break;
	case KEY_KP_EQUALS: scanout = SDL_SCANCODE_KP_EQUALS; break;
	case KEY_LCTRL: scanout = SDL_SCANCODE_LCTRL; break;
	case KEY_LSHIFT: scanout = SDL_SCANCODE_LSHIFT; break;
	case KEY_LALT: scanout = SDL_SCANCODE_LALT; break;
	case KEY_LGUI: scanout = SDL_SCANCODE_LGUI; break;
	case KEY_RCTRL: scanout = SDL_SCANCODE_RCTRL; break;
	case KEY_RSHIFT: scanout = SDL_SCANCODE_RSHIFT; break;
	case KEY_RALT: scanout = SDL_SCANCODE_RALT; break;
	case KEY_RGUI: scanout = SDL_SCANCODE_RGUI; break;
	*/
	default: scanout = -1; break;

	}

	return scanout;
}
