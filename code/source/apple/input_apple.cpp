#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <cstdio>

#include "input.h"
#include "tools.h"
#include "log.h"
#include "configuration.h"

namespace xs::input::internal
{
}

using namespace xs::input;

void xs::input::initialize()
{
}

void xs::input::shutdown()
{
}

void xs::input::update(double dt)
{
}

double xs::input::get_axis(gamepad_axis axis)
{
    return 0.0;
}

bool xs::input::get_button(gamepad_button button)
{
    return false;
}

bool xs::input::get_button_once(gamepad_button button)
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

bool xs::input::get_mousebutton(mouse_button button)
{
    return false;
}

bool xs::input::get_mousebutton_once(mouse_button button)
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

void xs::input::set_gamepad_vibration(int leftRumble, int rightRumble)
{
}

void xs::input::set_lightbar_color(double red, double green, double blue)
{
}

void xs::input::reset_lightbar()
{
}
