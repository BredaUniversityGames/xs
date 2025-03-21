#include "input.hpp"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include "log.hpp"
#include <nn/hid/hid_Npad.h>
#include <nn/hid/hid_NpadJoy.h>
#include <nn/hid/hid_ControllerSupport.h>
#include <nn/hid/hid_NpadColor.h>
#include <nn/hid/hid_TouchScreen.h>
#include <nn/util/util_Color.h>
#include <nn/hid/hid_Vibration.h>
#include <nn/nn_Result.h>
#include <nn/hid/hid_Result.controllerSupport.h>

#include "configuration.hpp"
#include "device.hpp"

enum class JoystickType
{
	XBOX,
	XBOX_360,
	PLAYSTATION,
	STEAM,
	//
	NINTENDO_LEFT_JOYCON,
	NINTENDO_RIGHT_JOYCON,
	NINTENDO_DUAL_JOYCONS,
	NINTENDO_FULLKEY,
	NINTENDO_HANDHELD,
	//
	VIRTUAL,
	//
	INVALID
};

namespace xs::input::internal
{
	enum InputState
	{
		P1 = 1,
		P2,
		P3,
		P4,
		P2to4
	};

	struct JoystickState
	{
		std::string			Name;			// The name of it
		std::vector<float>	Axes;			// All the axes
		std::vector<int>	Buttons;		// All the buttons		
		std::vector<float>	LastAxes;		// All the axes from previous update
		std::vector<int>	LastButtons;	// All the buttons from previous update
		bool				Connected;
		bool				Rumble;
		JoystickType		Type;

		JoystickState()
			: Axes(8), Buttons(24), LastAxes(8), LastButtons(24), Connected(true), Rumble(false), Type(JoystickType::INVALID)
		{}
	};

	float _timeOut = 0.0f;
	bool _isConfigurationValid;

	unsigned								_count;
	InputState								_state;		// is the last know mode singleplayer?
	std::map<int, JoystickState>			_joyState;	// Indexed by tokens
	//int										_nextVirtual = 256;
	//char									_keyOnce[256 + 1];
	//MouseState								_mouse;
	//bool									_rumble = true;
	nn::hid::TouchScreenState<nn::hid::TouchStateCountMax>	_touchScreenState;
	std::vector<std::pair<float, float>>	_touches_gameCoordinates;

	xs::configuration::scale_parameters touchScreenToGame;

	void AddJoystick(int joy);

	struct GamepadMapping
	{
		std::unordered_map<int, int> buttons;
		int leftTrigger;
		int rightTrigger;
	};

	GamepadMapping gamepadMapping_leftJoycon;
	GamepadMapping gamepadMapping_rightJoycon;
	GamepadMapping gamepadMapping_full;

}

using namespace std;
using namespace xs::input::internal;

void xs::input::initialize()
{
	nn::hid::InitializeNpad();

	// Configure valid play styles.
	nn::hid::SetSupportedNpadStyleSet(
		nn::hid::NpadStyleFullKey::Mask |
		nn::hid::NpadStyleJoyDual::Mask |
		nn::hid::NpadStyleJoyLeft::Mask |
		nn::hid::NpadStyleJoyRight::Mask |
		nn::hid::NpadStyleHandheld::Mask
	);

	// Configure the Joy-Con for solo horizontal grip.
	SetNpadJoyHoldType(nn::hid::NpadJoyHoldType::NpadJoyHoldType_Horizontal);

	// Add all possible controller states	
	AddJoystick(nn::hid::NpadId::No1);
	AddJoystick(nn::hid::NpadId::No2);
	AddJoystick(nn::hid::NpadId::No3);
	AddJoystick(nn::hid::NpadId::No4);
	AddJoystick(nn::hid::NpadId::Handheld);

	// Check which controllers are valid
	auto it = _joyState.begin();
	while (it != _joyState.end())
	{
		int joy = it->first;
		nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);

		if (!style.Test<nn::hid::NpadStyleJoyRight>() && !style.Test<nn::hid::NpadStyleJoyLeft>() &&
			!style.Test<nn::hid::NpadStyleJoyDual>() && !style.Test<nn::hid::NpadStyleFullKey>() &&
			!style.Test<nn::hid::NpadStyleHandheld>())
		{
			it = _joyState.erase(it);
		}
		else
		{
			++it;
			//InitializeVibrationDevice(joy);
		}
	}

	if (_joyState.size() == 0)
	{
		log::info("No valid controllers showing input menu...");
		//ShowInputMenu(P1);
	}
	else
	{
		switch (_joyState.size())
		{
		case 1: _state = P1; break;
		case 2: _state = P2; break;
		case 3: _state = P3; break;
		case 4: _state = P4; break;
		default: _state = P2to4; break;
		}
	}

	nn::hid::InitializeTouchScreen();

	//InitializeVibrationThread();

	// --- pre-define button mappings

	// left joycon only
	gamepadMapping_leftJoycon.buttons[gamepad_button::BUTTON_SOUTH] = nn::hid::NpadButton::Left::Index;
	gamepadMapping_leftJoycon.buttons[gamepad_button::BUTTON_EAST] = nn::hid::NpadButton::Down::Index;
	gamepadMapping_leftJoycon.buttons[gamepad_button::BUTTON_WEST] = nn::hid::NpadButton::Up::Index;
	gamepadMapping_leftJoycon.buttons[gamepad_button::BUTTON_NORTH] = nn::hid::NpadButton::Right::Index;
	gamepadMapping_leftJoycon.buttons[gamepad_button::BUTTON_START] = nn::hid::NpadButton::Minus::Index;
	gamepadMapping_leftJoycon.buttons[gamepad_button::STICK_LEFT] = nn::hid::NpadButton::StickL::Index;
	gamepadMapping_leftJoycon.leftTrigger = nn::hid::NpadJoyButton::LeftSL::Index;
	gamepadMapping_leftJoycon.rightTrigger = nn::hid::NpadJoyButton::LeftSR::Index;

	// right joycon only
	gamepadMapping_rightJoycon.buttons[gamepad_button::BUTTON_SOUTH] = nn::hid::NpadButton::A::Index;
	gamepadMapping_rightJoycon.buttons[gamepad_button::BUTTON_EAST] = nn::hid::NpadButton::X::Index;
	gamepadMapping_rightJoycon.buttons[gamepad_button::BUTTON_WEST] = nn::hid::NpadButton::B::Index;
	gamepadMapping_rightJoycon.buttons[gamepad_button::BUTTON_NORTH] = nn::hid::NpadButton::Y::Index;
	gamepadMapping_rightJoycon.buttons[gamepad_button::BUTTON_START] = nn::hid::NpadButton::Plus::Index;
	gamepadMapping_rightJoycon.buttons[gamepad_button::STICK_LEFT] = nn::hid::NpadButton::StickR::Index;
	gamepadMapping_rightJoycon.leftTrigger = nn::hid::NpadJoyButton::RightSL::Index;
	gamepadMapping_rightJoycon.rightTrigger = nn::hid::NpadJoyButton::RightSR::Index;

	// full layout (both joycons / handheld mode / pro controller)
	gamepadMapping_full.buttons[gamepad_button::BUTTON_SOUTH] = nn::hid::NpadButton::B::Index;
	gamepadMapping_full.buttons[gamepad_button::BUTTON_EAST] = nn::hid::NpadButton::A::Index;
	gamepadMapping_full.buttons[gamepad_button::BUTTON_WEST] = nn::hid::NpadButton::Y::Index;
	gamepadMapping_full.buttons[gamepad_button::BUTTON_NORTH] = nn::hid::NpadButton::X::Index;
	gamepadMapping_full.buttons[gamepad_button::BUTTON_START] = nn::hid::NpadButton::Plus::Index;
	gamepadMapping_full.buttons[gamepad_button::DPAD_LEFT] = nn::hid::NpadButton::Left::Index;
	gamepadMapping_full.buttons[gamepad_button::DPAD_DOWN] = nn::hid::NpadButton::Down::Index;
	gamepadMapping_full.buttons[gamepad_button::DPAD_UP] = nn::hid::NpadButton::Up::Index;
	gamepadMapping_full.buttons[gamepad_button::DPAD_RIGHT] = nn::hid::NpadButton::Right::Index;
	gamepadMapping_full.buttons[gamepad_button::BUTTON_SELECT] = nn::hid::NpadButton::Minus::Index;
	gamepadMapping_full.buttons[gamepad_button::STICK_LEFT] = nn::hid::NpadButton::StickL::Index;
	gamepadMapping_full.buttons[gamepad_button::STICK_RIGHT] = nn::hid::NpadButton::StickR::Index;
	gamepadMapping_full.buttons[gamepad_button::SHOULDER_LEFT] = nn::hid::NpadButton::L::Index;
	gamepadMapping_full.buttons[gamepad_button::SHOULDER_RIGHT] = nn::hid::NpadButton::L::Index;
	gamepadMapping_full.leftTrigger = nn::hid::NpadButton::ZL::Index;
	gamepadMapping_full.rightTrigger = nn::hid::NpadButton::ZR::Index;

	// --- precompute the mapping from touchscreen to game coordinates
	internal::touchScreenToGame = xs::configuration::get_scale_to_game(xs::device::get_width(), xs::device::get_height());
}

void xs::input::shutdown()
{
}

int GetDefaultJoystick()
{
	if (_joyState.empty())
		return 0;
	return _joyState.begin()->first;
}

float NormalizeStickInput(int32_t value)
{
	return value / float(nn::hid::AnalogStickMax);
}

void FillJoystickState(JoystickState& result_state, const nn::hid::NpadButtonSet& buttonSet, const GamepadMapping& mapping)
{
	for (auto& button : mapping.buttons)
		result_state.Buttons[button.first] = buttonSet.Test(button.second);

	result_state.Axes[xs::input::gamepad_axis::TRIGGER_LEFT] = buttonSet.Test(mapping.leftTrigger) ? 1.0f : 0.0f;
	result_state.Axes[xs::input::gamepad_axis::TRIGGER_RIGHT] = buttonSet.Test(mapping.rightTrigger) ? 1.0f : 0.0f;
}

void xs::input::update(double dt)
{
	// Check if there are any new controllers this frame
	AddJoystick(nn::hid::NpadId::No1);
	AddJoystick(nn::hid::NpadId::No2);
	AddJoystick(nn::hid::NpadId::No3);
	AddJoystick(nn::hid::NpadId::No4);
	AddJoystick(nn::hid::NpadId::Handheld);

	queue<int> removeQueue;
	_timeOut += dt;

	int connected = 0;
	_isConfigurationValid = true;
	for (auto& i : _joyState)
	{
		int joy = i.first;
		JoystickState& state = i.second;

		assert(nn::hid::GetNpadJoyHoldType() == nn::hid::NpadJoyHoldType_Horizontal);

		state.LastAxes.assign(state.Axes.begin(), state.Axes.end());
		state.LastButtons.assign(state.Buttons.begin(), state.Buttons.end());

		switch (state.Type) {
		case JoystickType::NINTENDO_LEFT_JOYCON:
		{
			nn::hid::NpadJoyLeftState leftState;
			nn::hid::GetNpadState(&leftState, joy);

			nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);
			if (style.Test<nn::hid::NpadStyleJoyLeft>())
			{
				connected++;
				FillJoystickState(state, leftState.buttons, gamepadMapping_leftJoycon);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_X] = -NormalizeStickInput(leftState.analogStickL.y);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_Y] = -NormalizeStickInput(leftState.analogStickL.x);
			}
			else
			{
				removeQueue.push(joy);
				state.Connected = false;
			}
		} break;
		case JoystickType::NINTENDO_RIGHT_JOYCON:
		{
			nn::hid::NpadJoyRightState rightState;
			nn::hid::GetNpadState(&rightState, joy);

			nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);
			if (style.Test<nn::hid::NpadStyleJoyRight>())
			{
				connected++;
				FillJoystickState(state, rightState.buttons, gamepadMapping_rightJoycon);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_X] = NormalizeStickInput(rightState.analogStickR.y);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_Y] = NormalizeStickInput(rightState.analogStickR.x);
			}
			else
			{
				removeQueue.push(joy);
				state.Connected = false;
			}
		} break;
		case JoystickType::NINTENDO_DUAL_JOYCONS:
		{
			nn::hid::NpadJoyDualState dualState;
			nn::hid::GetNpadState(&dualState, joy);

			nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);

			bool assure = dualState.attributes.Test<nn::hid::NpadJoyAttribute::IsLeftConnected>() &&
				dualState.attributes.Test<nn::hid::NpadJoyAttribute::IsRightConnected>();

			if (!assure)
				_isConfigurationValid = false;

			if (style.Test<nn::hid::NpadStyleJoyDual>())
			{
				connected++;
				FillJoystickState(state, dualState.buttons, gamepadMapping_full);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_X] = NormalizeStickInput(dualState.analogStickL.x);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_Y] = NormalizeStickInput(dualState.analogStickL.y);
				state.Axes[xs::input::gamepad_axis::STICK_RIGHT_X] = NormalizeStickInput(dualState.analogStickR.x);
				state.Axes[xs::input::gamepad_axis::STICK_RIGHT_Y] = NormalizeStickInput(dualState.analogStickR.y);
			}
			else
			{
				removeQueue.push(joy);
				state.Connected = false;
			}
		} break;
		case JoystickType::NINTENDO_FULLKEY:
		{
			nn::hid::NpadFullKeyState fullState;
			nn::hid::GetNpadState(&fullState, joy);

			nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);
			if (style.Test<nn::hid::NpadStyleFullKey>())
			{
				connected++;
				FillJoystickState(state, fullState.buttons, gamepadMapping_full);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_X] = NormalizeStickInput(fullState.analogStickL.x);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_Y] = NormalizeStickInput(fullState.analogStickL.y);
				state.Axes[xs::input::gamepad_axis::STICK_RIGHT_X] = NormalizeStickInput(fullState.analogStickR.x);
				state.Axes[xs::input::gamepad_axis::STICK_RIGHT_Y] = NormalizeStickInput(fullState.analogStickR.y);
			}
			else
			{
				removeQueue.push(joy);
				state.Connected = false;
			}
		} break;
		case JoystickType::NINTENDO_HANDHELD:
		{
			nn::hid::NpadHandheldState handheldState;
			nn::hid::GetNpadState(&handheldState, joy);
			nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);
			if (style.Test<nn::hid::NpadStyleHandheld>())
			{
				connected++;
				FillJoystickState(state, handheldState.buttons, gamepadMapping_full);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_X] = NormalizeStickInput(handheldState.analogStickL.x);
				state.Axes[xs::input::gamepad_axis::STICK_LEFT_Y] = -NormalizeStickInput(handheldState.analogStickL.y);
				state.Axes[xs::input::gamepad_axis::STICK_RIGHT_X] = NormalizeStickInput(handheldState.analogStickR.x);
				state.Axes[xs::input::gamepad_axis::STICK_RIGHT_Y] = -NormalizeStickInput(handheldState.analogStickR.y);

				if (_joyState.size() > 1)
					removeQueue.push(joy);
			}
			else
			{
				removeQueue.push(joy);
				state.Connected = false;
			}
		} break;
		default:
			;
		}
	}

	// Remove disconnected/invalid combinations
	while (!removeQueue.empty())
	{
		int joy = removeQueue.front();
		const auto& state = _joyState[joy];
		log::info("Joystick {} disconnected.", state.Name.c_str());
		_joyState.erase(joy);
		removeQueue.pop();
	}

	// Get touchscreen data
	nn::hid::GetTouchScreenState(&_touchScreenState);

	// Translate touch points to game coordinates
	_touches_gameCoordinates.resize(_touchScreenState.count);
	for (auto i = 0; i < _touchScreenState.count; ++i)
		xs::configuration::scale_to_game(_touchScreenState.touches[i].x, _touchScreenState.touches[i].y, internal::touchScreenToGame, _touches_gameCoordinates[i].first, _touches_gameCoordinates[i].second);

	if (connected == 0)
		xs::log::warn("No controllers connected!");
	
	// Check if there are any new controllers this frame
	//AddJoystick(nn::hid::NpadId::No1);
	//AddJoystick(nn::hid::NpadId::No2);
	//AddJoystick(nn::hid::NpadId::No3);
	//AddJoystick(nn::hid::NpadId::No4);
	//AddJoystick(nn::hid::NpadId::Handheld);

}

double xs::input::get_axis(gamepad_axis axis)
{
	auto joystick = GetDefaultJoystick();
	auto itr = _joyState.find(joystick);
	if (itr != _joyState.end())
	{
		auto& state = itr->second;
		return state.Axes[axis];
	}
	return 0.0f;
}

bool xs::input::get_button(gamepad_button button)
{
	auto joystick = GetDefaultJoystick();
	auto itr = _joyState.find(joystick);
	if (itr != _joyState.end())
	{
		auto& state = itr->second;
		return state.Buttons[button];
	}
	return false;

}

bool xs::input::get_button_once(gamepad_button button)
{
	auto joystick = GetDefaultJoystick();
	auto itr = _joyState.find(joystick);
	if (itr != _joyState.end())
	{
		auto& state = itr->second;
		return	state.Buttons[button] && !state.LastButtons[button];
	}
	return false;

}

bool xs::input::get_key(int key)
{
	return false;
}

bool xs::input::get_key_once(int key)
{
	return false;
}

bool xs::input::get_mouse()
{
	return false;
}

double xs::input::get_mouse_x()
{
	return 0.0;
}

double xs::input::get_mouse_y()
{
	return 0.0;
}

bool xs::input::get_mousebutton(mouse_button button)
{
	return false;
}

bool xs::input::get_mousebutton_once(mouse_button button)
{
	return false;
}

double xs::input::get_mouse_wheel()
{
	return 0.0;
}

int xs::input::get_nr_touches()
{
	return _touchScreenState.count;
}

int xs::input::get_touch_id(int index)
{
	if (index >= 0 && index < _touchScreenState.count)
		return _touchScreenState.touches[index].fingerId;
	return -1;
}

double xs::input::get_touch_x(int index)
{
	if (index >= 0 && index < _touchScreenState.count)
		return static_cast<double>(_touches_gameCoordinates[index].first);
	return 0.0;
}

double xs::input::get_touch_y(int index)
{
	if (index >= 0 && index < _touchScreenState.count)
		return static_cast<double>(_touches_gameCoordinates[index].second);
	return 0.0;
}

//These three functions only exists here to not throw an error when called
void xs::input::set_gamepad_vibration(double smallRumble, double largeRumble, double time)
{
	//Unimplemented on the switch (Probably too simple for the switch rumble)
}

void xs::input::set_lightbar_color(double red, double green, double blue)
{
	//Unimplemented on the switch (specific dualshock 5 controller mechanic)
}

void xs::input::reset_lightbar()
{
	//Unimplemented on the switch (specific dualshock 5 controller mechanic)
}


std::string GetName(JoystickType type)
{
	switch (type)
	{
	case JoystickType::NINTENDO_LEFT_JOYCON: return string("Left Joycon");
	case JoystickType::NINTENDO_RIGHT_JOYCON: return string("Right Joycon");
	case JoystickType::NINTENDO_DUAL_JOYCONS: return string("Dual Joycons");
	case JoystickType::NINTENDO_FULLKEY: return string("Full Key (Pro)");
	case JoystickType::NINTENDO_HANDHELD: return string("Handheld");
	default: return string("");
	}
}

std::string GetNpadIdName(uint32_t id)
{
	switch (id)
	{
	case nn::hid::NpadId::No1:		return string("No1");
	case nn::hid::NpadId::No2:		return string("No2");
	case nn::hid::NpadId::No3:		return string("No3");
	case nn::hid::NpadId::No4:		return string("No4");
	case nn::hid::NpadId::Handheld: return string("Handheld");
	default: return string("Unknown");
	}
}

void xs::input::internal::AddJoystick(int joy)
{
	// Don't add handheld when there are other controllers present (per Nintendo recommendations)
	if (joy == nn::hid::NpadId::Handheld && !_joyState.empty())
		return;

	JoystickState state;

	assert(nn::hid::GetNpadJoyHoldType() == nn::hid::NpadJoyHoldType_Horizontal); // Osmium limitation

	nn::hid::NpadStyleSet style = nn::hid::GetNpadStyleSet(joy);

	if (style.Test<nn::hid::NpadStyleJoyRight>())
	{
		state.Type = JoystickType::NINTENDO_RIGHT_JOYCON;
	}
	else if (style.Test<nn::hid::NpadStyleJoyLeft>())
	{
		state.Type = JoystickType::NINTENDO_LEFT_JOYCON;
	}
	else if (style.Test<nn::hid::NpadStyleJoyDual>())
	{
		state.Type = JoystickType::NINTENDO_DUAL_JOYCONS;
	}
	else if (style.Test<nn::hid::NpadStyleFullKey>()) // (Pro)
	{
		state.Type = JoystickType::NINTENDO_FULLKEY;
	}
	else if (style.Test<nn::hid::NpadStyleHandheld>())
	{
		if (_joyState.empty())
			state.Type = JoystickType::NINTENDO_HANDHELD;
	}
	else
	{
		state.Type = JoystickType::INVALID;
	}

	if (state.Type != JoystickType::INVALID)
	{
		if (_joyState.find(joy) == _joyState.end())
		{
			state.Name = GetName(state.Type) + " @ " + GetNpadIdName(joy);
			_joyState[joy] = state;
			//InitializeVibrationDevice(joy);
			log::info("Joystick %s connected.", state.Name.c_str());
		}
		else
		{
			_joyState[joy].Type = state.Type;
		}
	}
}
