#include "input.h"
#include "tools.h"
#include "log.h"
#include "configuration.h"

#include <cstdlib>
#include <cstdio>
#include <libsysmodule.h>
#include <pad.h>

#include <vector>
#include <unordered_map>
#include <string>

namespace xs::input::internal
{
	int32_t handle;
	SceUserServiceUserId userId;

	ScePadData data;
	ScePadData previousData;

	std::unordered_map<int, int> buttonMapping;
	std::unordered_map<int, int> axisMapping;

	std::vector<std::pair<float, float>> touches_gameCoordinates;
	xs::configuration::scale_parameters touchpadToGame;
}

using namespace xs::input;

void xs::input::initialize()
{
	sceUserServiceInitialize(NULL);
	SceUserServiceUserId userId;
	// Get user ID value
	auto ret = sceUserServiceGetInitialUser(&userId);
	if (ret < 0) {
		/* Failed to obtain user ID value */
		return;
	}

	scePadInit();
	//  Example that specifies SCE_PAD_PORT_TYPE_STANDARD to type
	// Specify 0 for index
	// pParam is a reserved area (specify NULL)
	internal::handle = scePadOpen(userId, SCE_PAD_PORT_TYPE_STANDARD, 0, NULL);
	if (internal::handle < 0)
	{
		return;
		/* Setting failed */
	}

	// get the size of the touchpad

	ScePadControllerInformation info;
	scePadGetControllerInformation(internal::handle, &info);
	internal::touchpadToGame = xs::configuration::get_scale_to_game(info.touchPadInfo.resolution.x, info.touchPadInfo.resolution.y);

	// define the mapping from generic xs buttons to ScePad buttons

	internal::buttonMapping[xs::input::gamepad_button::BUTTON_SOUTH] = SCE_PAD_BUTTON_CROSS;
	internal::buttonMapping[xs::input::gamepad_button::BUTTON_EAST] = SCE_PAD_BUTTON_CIRCLE;
	internal::buttonMapping[xs::input::gamepad_button::BUTTON_WEST] = SCE_PAD_BUTTON_SQUARE;
	internal::buttonMapping[xs::input::gamepad_button::BUTTON_NORTH] = SCE_PAD_BUTTON_TRIANGLE;

	internal::buttonMapping[xs::input::gamepad_button::SHOULDER_LEFT] = SCE_PAD_BUTTON_L1;
	internal::buttonMapping[xs::input::gamepad_button::SHOULDER_RIGHT] = SCE_PAD_BUTTON_R1;
	internal::buttonMapping[xs::input::gamepad_button::BUTTON_START] = SCE_PAD_BUTTON_OPTIONS;
	internal::buttonMapping[xs::input::gamepad_button::STICK_LEFT] = SCE_PAD_BUTTON_L3;
	internal::buttonMapping[xs::input::gamepad_button::STICK_RIGHT] = SCE_PAD_BUTTON_R3;

	internal::buttonMapping[xs::input::gamepad_button::DPAD_UP] = SCE_PAD_BUTTON_UP;
	internal::buttonMapping[xs::input::gamepad_button::DPAD_RIGHT] = SCE_PAD_BUTTON_RIGHT;
	internal::buttonMapping[xs::input::gamepad_button::DPAD_DOWN] = SCE_PAD_BUTTON_DOWN;
	internal::buttonMapping[xs::input::gamepad_button::DPAD_LEFT] = SCE_PAD_BUTTON_LEFT;

	// Notes:
	// - An equivalent for BUTTON_SELECT does not exist on the PS5.
	// - The left and right trigger (L2/R2) are axes in xs::input instead of buttons.
	//   On PS5, they occur as both axes and buttons, but we ignore the button component here.
}

void xs::input::shutdown()
{
	scePadClose(internal::handle);
}

void xs::input::update(double dt)
{
	internal::previousData = internal::data;
	scePadReadState(internal::handle, &internal::data);

	// translate touches to game coordinates
	internal::touches_gameCoordinates.resize(internal::data.touchData.touchNum);
	for (int i = 0; i < internal::data.touchData.touchNum; ++i)
	{
		xs::configuration::scale_to_game(internal::data.touchData.touch[i].x, internal::data.touchData.touch[i].y, internal::touchpadToGame, internal::touches_gameCoordinates[i].first, internal::touches_gameCoordinates[i].second);
	}
}

double xs::input::get_axis(int axis)
{
	// left stick: -1 to 1
	if (axis == xs::input::gamepad_axis::STICK_LEFT_X)
		return (internal::data.leftStick.x - 128) / 128.0;
	if (axis == xs::input::gamepad_axis::STICK_LEFT_Y)
		return (128 - internal::data.leftStick.y) / 128.0;

	// right stick: -1 to 1
	if (axis == xs::input::gamepad_axis::STICK_RIGHT_X)
		return (internal::data.rightStick.x - 128) / 128.0;
	if (axis == xs::input::gamepad_axis::STICK_RIGHT_Y)
		return (128 - internal::data.rightStick.y) / 128.0;

	// triggers: 0 to 1
	if (axis == xs::input::gamepad_axis::TRIGGER_LEFT)
		return internal::data.analogButtons.l2 / 255.0;
	if (axis == xs::input::gamepad_axis::TRIGGER_RIGHT)
		return internal::data.analogButtons.r2 / 255.0;

	// unknown
	return 0.0;
}

bool xs::input::get_button(int button)
{
	// map to the correct ScePad button
	auto it = xs::input::internal::buttonMapping.find(button);
	if (it == xs::input::internal::buttonMapping.end())
		return false;

	return xs::tools::check_bit_flag_overlap(internal::data.buttons, it->second);
}

bool xs::input::get_button_once(int button)
{
	// map to the correct ScePad button
	auto it = xs::input::internal::buttonMapping.find(button);
	if (it == xs::input::internal::buttonMapping.end())
		return false;

	return xs::tools::check_bit_flag_overlap(internal::data.buttons, it->second)
		&& !xs::tools::check_bit_flag_overlap(internal::previousData.buttons, it->second);
}

bool xs::input::get_key(int key)
{
	return false;
}

bool xs::input::get_key_once(int key)
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

bool xs::input::get_mouse()
{
	return false;
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
	return internal::data.touchData.touchNum;
}

int xs::input::get_touch_id(int index)
{
	if (index >= internal::data.touchData.touchNum)
		return -1;
	return internal::data.touchData.touch[index].id;
}

double xs::input::get_touch_x(int index)
{
	if (index >= internal::data.touchData.touchNum)
		return 0.0;

	// TODO: scale to game coordinates
	return static_cast<double>(internal::touches_gameCoordinates[index].first);
}

double xs::input::get_touch_y(int index)
{
	if (index >= internal::data.touchData.touchNum)
		return 0.0;

	// TODO: scale to game coordinates
	return static_cast<double>(internal::touches_gameCoordinates[index].second);
}