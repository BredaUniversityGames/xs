#include "input.h"
#include "tools.h"
#include <cstdlib>
#include <cstdio>
#include <libsysmodule.h>
#include <pad.h>
#include "log.h"

namespace xs::input::internal
{
	int32_t handle;
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
}

void xs::input::shutdown()
{
}

void xs::input::update(double dt)
{
	ScePadData data;
	scePadReadState(internal::handle, &data);
	if (xs::tools::check_bit_flag_overlap(data.buttons, SCE_PAD_BUTTON_CROSS))
	{
		xs::log::info("whee!");
	}
}

double xs::input::get_axis(int axis)
{
	return 0.0;
}

bool xs::input::get_button(int button)
{
	return false;
}

bool xs::input::get_button_once(int button)
{
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