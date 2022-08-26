#include "input.h"
#include <string>
#include <vector>
#include <map>
#include <queue>
#include "log.h"
#include <nn/hid/hid_Npad.h>
#include <nn/hid/hid_NpadJoy.h>
#include <nn/hid/hid_ControllerSupport.h>
#include <nn/hid/hid_NpadColor.h>
#include <nn/hid/hid_TouchScreen.h>
#include <nn/util/util_Color.h>
#include <nn/hid/hid_Vibration.h>
#include <nn/nn_Result.h>
#include <nn/hid/hid_Result.controllerSupport.h>

#include "configuration.h"
#include "device.h"

enum JoystickButtons
{
	// Default order matching Xbox (360 and One) controller
	JOYSTICK_BUTTON_A = 0,
	JOYSTICK_BUTTON_B = 1,
	JOYSTICK_BUTTON_X = 2,
	JOYSTICK_BUTTON_Y = 3,
	JOYSTICK_BUTTON_LB = 4,
	JOYSTICK_BUTTON_RB = 5,
	JOYSTICK_BUTTON_BACK = 6,
	JOYSTICK_BUTTON_START = 7,
	JOYSTICK_BUTTON_DUMMY = 8,
	JOYSTICK_BUTTON_L3 = 9,
	JOYSTICK_BUTTON_R3 = 10,
	JOYSTICK_BUTTON_DPAD_UP = 11,
	JOYSTICK_BUTTON_DPAD_RIGHT = 12,
	JOYSTICK_BUTTON_DPAD_DOWN = 13,
	JOYSTICK_BUTTON_DPAD_LEFT = 14,
	JOYSTICK_BUTTON_COUNT
};

enum JoystickAxes
{
	// Default order matching Xbox (360 and One) controller
	JOYSTICK_AXIS_LEFT_THUMB_HORIZONTAL = 0,
	JOYSTICK_AXIS_LEFT_THUMB_VERTICAL,			// 1
	JOYSTICK_AXIS_RIGHT_THUMB_HORIZONTAL,		// 2
	JOYSTICK_AXIS_RIGHT_THUMB_VERTICAL,			// 3
	JOYSTICK_AXIS_LEFT_TRIGGER,					// 4
	JOYSTICK_AXIS_RIGHT_TRIGGER,				// 5	
	JOYSTICK_AXIS_COUNT							// 6
};

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

	void AddJoystick(int joy);
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
				state.Buttons[JOYSTICK_BUTTON_B] = leftState.buttons.Test<nn::hid::NpadButton::Left>();
				state.Buttons[JOYSTICK_BUTTON_Y] = leftState.buttons.Test<nn::hid::NpadButton::Up>();
				state.Buttons[JOYSTICK_BUTTON_A] = leftState.buttons.Test<nn::hid::NpadButton::Down>();
				state.Buttons[JOYSTICK_BUTTON_X] = leftState.buttons.Test<nn::hid::NpadButton::Right>();
				state.Buttons[JOYSTICK_BUTTON_START] = leftState.buttons.Test<nn::hid::NpadButton::Minus>();
				state.Axes[JOYSTICK_AXIS_RIGHT_TRIGGER] = leftState.buttons.Test<nn::hid::NpadJoyButton::LeftSR>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_TRIGGER] = leftState.buttons.Test<nn::hid::NpadJoyButton::LeftSL>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_HORIZONTAL] = float(-leftState.analogStickL.y) / float(nn::hid::AnalogStickMax);
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_VERTICAL] = float(-leftState.analogStickL.x) / float(nn::hid::AnalogStickMax);
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
				state.Buttons[JOYSTICK_BUTTON_B] = rightState.buttons.Test<nn::hid::NpadButton::A>();
				state.Buttons[JOYSTICK_BUTTON_Y] = rightState.buttons.Test<nn::hid::NpadButton::B>();
				state.Buttons[JOYSTICK_BUTTON_A] = rightState.buttons.Test<nn::hid::NpadButton::X>();
				state.Buttons[JOYSTICK_BUTTON_X] = rightState.buttons.Test<nn::hid::NpadButton::Y>();
				state.Buttons[JOYSTICK_BUTTON_START] = rightState.buttons.Test<nn::hid::NpadButton::Plus>();
				state.Axes[JOYSTICK_AXIS_RIGHT_TRIGGER] = rightState.buttons.Test<nn::hid::NpadJoyButton::RightSR>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_TRIGGER] = rightState.buttons.Test<nn::hid::NpadJoyButton::RightSL>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_HORIZONTAL] = float(rightState.analogStickR.y) / float(nn::hid::AnalogStickMax);
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_VERTICAL] = float(rightState.analogStickR.x) / float(nn::hid::AnalogStickMax);
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
				state.Buttons[JOYSTICK_BUTTON_A] = dualState.buttons.Test<nn::hid::NpadButton::A>();
				state.Buttons[JOYSTICK_BUTTON_B] = dualState.buttons.Test<nn::hid::NpadButton::B>();
				state.Buttons[JOYSTICK_BUTTON_X] = dualState.buttons.Test<nn::hid::NpadButton::X>();
				state.Buttons[JOYSTICK_BUTTON_Y] = dualState.buttons.Test<nn::hid::NpadButton::Y>();
				state.Buttons[JOYSTICK_BUTTON_START] = dualState.buttons.Test<nn::hid::NpadButton::Plus>();
				state.Buttons[JOYSTICK_BUTTON_BACK] = dualState.buttons.Test<nn::hid::NpadButton::Minus>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_UP] = dualState.buttons.Test<nn::hid::NpadButton::Up>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_DOWN] = dualState.buttons.Test<nn::hid::NpadButton::Down>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_LEFT] = dualState.buttons.Test<nn::hid::NpadButton::Left>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_RIGHT] = dualState.buttons.Test<nn::hid::NpadButton::Right>();
				state.Axes[JOYSTICK_AXIS_RIGHT_TRIGGER] = dualState.buttons.Test<nn::hid::NpadButton::ZR>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_TRIGGER] = dualState.buttons.Test<nn::hid::NpadButton::ZL>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_HORIZONTAL] = float(dualState.analogStickL.x) / float(nn::hid::AnalogStickMax);
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_VERTICAL] = float(-dualState.analogStickL.y) / float(nn::hid::AnalogStickMax);
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
				state.Buttons[JOYSTICK_BUTTON_A] = fullState.buttons.Test<nn::hid::NpadButton::A>();
				state.Buttons[JOYSTICK_BUTTON_B] = fullState.buttons.Test<nn::hid::NpadButton::B>();
				state.Buttons[JOYSTICK_BUTTON_X] = fullState.buttons.Test<nn::hid::NpadButton::X>();
				state.Buttons[JOYSTICK_BUTTON_Y] = fullState.buttons.Test<nn::hid::NpadButton::Y>();
				state.Buttons[JOYSTICK_BUTTON_START] = fullState.buttons.Test<nn::hid::NpadButton::Plus>();
				state.Buttons[JOYSTICK_BUTTON_BACK] = fullState.buttons.Test<nn::hid::NpadButton::Minus>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_UP] = fullState.buttons.Test<nn::hid::NpadButton::Up>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_DOWN] = fullState.buttons.Test<nn::hid::NpadButton::Down>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_LEFT] = fullState.buttons.Test<nn::hid::NpadButton::Left>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_RIGHT] = fullState.buttons.Test<nn::hid::NpadButton::Right>();
				state.Axes[JOYSTICK_AXIS_RIGHT_TRIGGER] = fullState.buttons.Test<nn::hid::NpadButton::ZR>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_TRIGGER] = fullState.buttons.Test<nn::hid::NpadButton::ZL>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_HORIZONTAL] = float(fullState.analogStickL.x) / float(nn::hid::AnalogStickMax);
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_VERTICAL] = float(-fullState.analogStickL.y) / float(nn::hid::AnalogStickMax);
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
				state.Buttons[JOYSTICK_BUTTON_A] = handheldState.buttons.Test<nn::hid::NpadButton::A>();
				state.Buttons[JOYSTICK_BUTTON_B] = handheldState.buttons.Test<nn::hid::NpadButton::B>();
				state.Buttons[JOYSTICK_BUTTON_X] = handheldState.buttons.Test<nn::hid::NpadButton::X>();
				state.Buttons[JOYSTICK_BUTTON_Y] = handheldState.buttons.Test<nn::hid::NpadButton::Y>();
				state.Buttons[JOYSTICK_BUTTON_START] = handheldState.buttons.Test<nn::hid::NpadButton::Plus>();
				state.Buttons[JOYSTICK_BUTTON_BACK] = handheldState.buttons.Test<nn::hid::NpadButton::Minus>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_UP] = handheldState.buttons.Test<nn::hid::NpadButton::Up>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_DOWN] = handheldState.buttons.Test<nn::hid::NpadButton::Down>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_LEFT] = handheldState.buttons.Test<nn::hid::NpadButton::Left>();
				state.Buttons[JOYSTICK_BUTTON_DPAD_RIGHT] = handheldState.buttons.Test<nn::hid::NpadButton::Right>();
				state.Axes[JOYSTICK_AXIS_RIGHT_TRIGGER] = handheldState.buttons.Test<nn::hid::NpadButton::ZR>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_TRIGGER] = handheldState.buttons.Test<nn::hid::NpadButton::ZL>() ? 1.0f : 0.0f;
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_HORIZONTAL] = float(handheldState.analogStickL.x) / float(nn::hid::AnalogStickMax);
				state.Axes[JOYSTICK_AXIS_LEFT_THUMB_VERTICAL] = float(-handheldState.analogStickL.y) / float(nn::hid::AnalogStickMax);

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
	// Translate screen to game canvas coordinates
	_touches_gameCoordinates.resize(_touchScreenState.count);
	for (auto i = 0; i < _touchScreenState.count; ++i)
		xs::device::screen_to_game(_touchScreenState.touches[i].x, _touchScreenState.touches[i].y, _touches_gameCoordinates[i].first, _touches_gameCoordinates[i].second);

	// Check if there are any new controllers this frame
	//AddJoystick(nn::hid::NpadId::No1);
	//AddJoystick(nn::hid::NpadId::No2);
	//AddJoystick(nn::hid::NpadId::No3);
	//AddJoystick(nn::hid::NpadId::No4);
	//AddJoystick(nn::hid::NpadId::Handheld);

}

double xs::input::get_axis(int axis)
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

bool xs::input::get_button(int button)
{
	auto joystick = GetDefaultJoystick();
	auto itr = _joyState.find(joystick);
	if (itr != _joyState.end())
	{
		auto& state = itr->second;
		return	state.Buttons[button];
	}
	return false;

}

bool xs::input::get_button_once(int button)
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

bool xs::input::get_mousebutton(int button)
{
	return false;
}

bool xs::input::get_mousebutton_once(int button)
{
	return false;
}

int xs::input::get_nr_touches()
{
	return _touchScreenState.count;
}

int xs::input::get_touch_id(int index)
{
	if (index >= 0 && index < _touchScreenState.count)
		return _touchScreenState.touches[index].fingerId;
	return 0;
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
