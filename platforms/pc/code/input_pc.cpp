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

void xs::input::set_gamepad_vibration(double low, double high, double time)
{
#ifdef DEBUG
	if (data::get_bool("Vibration.Disabled", data::type::debug))
		return;
#endif
	
	auto gamepad = get_default_gamepad();
	if (gamepad == nullptr)
		return;

	uint16 low_rumble = (uint16)(low * 0xFFFF);
	uint16 high_rumble = (uint16)(high * 0xFFFF);
	SDL_RumbleGamepad(gamepad->sdl_pad, high_rumble, low_rumble, (uint32)(time * 1000));
}

void xs::input::set_lightbar_color(double red, double green, double blue) {}
void xs::input::reset_lightbar() {}

bool xs::input::get_key(int key)
{
	return 
}
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
