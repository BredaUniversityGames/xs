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

	ScePadVibrationParam rumbleParameter;
	int maxRumbleIntensity = 255;
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

	internal::buttonMapping[gamepad_button::BUTTON_SOUTH] = SCE_PAD_BUTTON_CROSS;
	internal::buttonMapping[gamepad_button::BUTTON_EAST] = SCE_PAD_BUTTON_CIRCLE;
	internal::buttonMapping[gamepad_button::BUTTON_WEST] = SCE_PAD_BUTTON_SQUARE;
	internal::buttonMapping[gamepad_button::BUTTON_NORTH] = SCE_PAD_BUTTON_TRIANGLE;

	internal::buttonMapping[gamepad_button::SHOULDER_LEFT] = SCE_PAD_BUTTON_L1;
	internal::buttonMapping[gamepad_button::SHOULDER_RIGHT] = SCE_PAD_BUTTON_R1;
	internal::buttonMapping[gamepad_button::BUTTON_START] = SCE_PAD_BUTTON_OPTIONS;
	internal::buttonMapping[gamepad_button::STICK_LEFT] = SCE_PAD_BUTTON_L3;
	internal::buttonMapping[gamepad_button::STICK_RIGHT] = SCE_PAD_BUTTON_R3;

	internal::buttonMapping[gamepad_button::DPAD_UP] = SCE_PAD_BUTTON_UP;
	internal::buttonMapping[gamepad_button::DPAD_RIGHT] = SCE_PAD_BUTTON_RIGHT;
	internal::buttonMapping[gamepad_button::DPAD_DOWN] = SCE_PAD_BUTTON_DOWN;
	internal::buttonMapping[gamepad_button::DPAD_LEFT] = SCE_PAD_BUTTON_LEFT;

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
		xs::configuration::scale_to_game(
			internal::data.touchData.touch[i].x,
			internal::data.touchData.touch[i].y,
			internal::touchpadToGame,
			internal::touches_gameCoordinates[i].first,
			internal::touches_gameCoordinates[i].second);
	}
}

double xs::input::get_axis(gamepad_axis axis)
{
	// left stick: -1 to 1
	if (axis == gamepad_axis::STICK_LEFT_X)
		return (internal::data.leftStick.x - 128) / 128.0;
	if (axis == gamepad_axis::STICK_LEFT_Y)
		return (internal::data.leftStick.y - 128) / 128.0;

	// right stick: -1 to 1
	if (axis == gamepad_axis::STICK_RIGHT_X)
		return (internal::data.rightStick.x - 128) / 128.0;
	if (axis == gamepad_axis::STICK_RIGHT_Y)
		return (internal::data.rightStick.y - 128) / 128.0;

	// triggers: 0 to 1
	if (axis == gamepad_axis::TRIGGER_LEFT)
		return internal::data.analogButtons.l2 / 255.0;
	if (axis == gamepad_axis::TRIGGER_RIGHT)
		return internal::data.analogButtons.r2 / 255.0;

	// unknown
	return 0.0;
}

bool xs::input::get_button(gamepad_button button)
{
	// map to the correct ScePad button
	auto it = xs::input::internal::buttonMapping.find(button);
	if (it == xs::input::internal::buttonMapping.end())
		return false;

	return xs::tools::check_bit_flag_overlap(internal::data.buttons, it->second);
}

bool xs::input::get_button_once(gamepad_button button)
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

void xs::input::set_gamepad_vibration(int leftRumble, int rightRumble)
{
	if (leftRumble > 1)
		leftRumble = 1;
	else if (leftRumble < 0)
		leftRumble = 0;

	if (rightRumble > 1)
		rightRumble = 1;
	else if (rightRumble < 0)
		rightRumble = 0;

	//Note:
	//Technically there are 2 compatablity modes for PS5 pad vibration.
	//We use the one that is most faithfull to the dualshock 4 controllers rumble.
	scePadSetVibrationMode(internal::handle, SCE_PAD_VIBRATION_MODE_COMPATIBLE2);

	
	internal::rumbleParameter.smallMotor = rightRumble * internal::maxRumbleIntensity;
	internal::rumbleParameter.largeMotor = leftRumble * internal::maxRumbleIntensity;

	if (scePadSetVibration(internal::handle, &internal::rumbleParameter) < 0)
	{
		//Note: Removed error code printing since this wasnt done anywhere else
		//Negative values mean an error, the meaning of the code can be found in the playstation documentation
	}
}

void xs::input::set_lightbar_color(double red, double green, double blue)
{
	//Todo: maby use std clamp
	//Clamp the RGB colours so they dont exeed 255 or get lower than 0
	if (red > 255)
		red = 255;
	else if (red < 0)
		red = 0;

	if (green > 255)
		green = 255;
	else if (green < 0)
		green = 0;

	if (blue > 255)
		blue = 255;
	else if (blue < 0)
		blue = 0;

	ScePadLightBarParam lightbarParameter;
	lightbarParameter.r = red;
	lightbarParameter.g = green;
	lightbarParameter.b = blue;

	scePadSetLightBar(internal::handle, &lightbarParameter);
}

void xs::input::reset_lightbar()
{
	scePadResetLightBar(internal::handle);
}