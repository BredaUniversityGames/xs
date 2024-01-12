#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <cstdio>

#include "input.h"
#include "tools.h"
#include "log.h"
#include "configuration.h"

#import <GameController/GameController.h>

namespace xs::input::internal
{
    GCExtendedGamepad* gamepad = nil;
    GCExtendedGamepad* gamepad_prev_frame = nil;
    bool get_button_pressed(GCExtendedGamepad* pad, gamepad_button button);
}

using namespace xs::input::internal;

void xs::input::initialize()
{
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"Finished finding controllers");
    }];
}

void xs::input::shutdown()
{
    [GCController stopWirelessControllerDiscovery];
}

void xs::input::update(double dt)
{
    auto controllers = [GCController controllers];
    
    if(gamepad)
        gamepad_prev_frame = gamepad.capture;
    
    gamepad = nil;
    if(controllers.count > 0) {
        GCController* controller = controllers[0];
        gamepad = controller.extendedGamepad.capture;
    }
}

double xs::input::get_axis(gamepad_axis axis)
{
    if (gamepad == nil)
        return 0.0;
    
    switch (axis)
    {
        case STICK_LEFT_X:
            return gamepad.leftThumbstick.xAxis.value;
        case STICK_LEFT_Y:
            return -gamepad.leftThumbstick.yAxis.value;
        case STICK_RIGHT_X:
            return gamepad.rightThumbstick.xAxis.value;
        case STICK_RIGHT_Y:
            return -gamepad.rightThumbstick.yAxis.value;
        case TRIGGER_LEFT:
            return gamepad.leftTrigger.value;
        case TRIGGER_RIGHT:
            return gamepad.rightTrigger.value;
        default:
            return 0.0f;
    }
}

bool xs::input::get_button(gamepad_button button)
{
    if (gamepad == nil)
        return false;
    
    return get_button_pressed(gamepad, button);
}

bool xs::input::get_button_once(gamepad_button button)
{
    if (gamepad == nil || gamepad_prev_frame == nil)
        return false;
    
    bool cur = get_button_pressed(gamepad, button);
    bool prv = get_button_pressed(gamepad_prev_frame, button);
    return cur && !prv;
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

void xs::input::reset_lightbar() {}


bool xs::input::internal::get_button_pressed(GCExtendedGamepad* pad, gamepad_button button)
{
    switch(button)
    {
        case BUTTON_SOUTH:
            return pad.buttonA.pressed;
        case BUTTON_EAST:
            return pad.buttonB.pressed;
        case BUTTON_WEST:
            return pad.buttonX.pressed;
        case BUTTON_NORTH:
            return pad.buttonY.pressed;
        case SHOULDER_LEFT:
            return pad.leftShoulder.pressed;
        case SHOULDER_RIGHT:
            return pad.rightShoulder.pressed;
        case BUTTON_SELECT:        
            return pad.buttonOptions.pressed;
        case BUTTON_START:
            return pad.buttonMenu.pressed;
        case STICK_LEFT:
            return pad.leftThumbstickButton.pressed;
        case STICK_RIGHT:
            return pad.rightThumbstickButton.pressed;
        case DPAD_UP:
            return pad.dpad.up.pressed;
        case DPAD_DOWN:
            return pad.dpad.down.pressed;
        case DPAD_LEFT:
            return pad.dpad.left.pressed;
        case DPAD_RIGHT:
            return pad.dpad.right.pressed;
        default:
            return false;
    }
}
