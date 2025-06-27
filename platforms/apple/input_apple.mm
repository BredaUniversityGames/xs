#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <cstdio>

#include "input.hpp"
#include "tools.hpp"
#include "log.hpp"
#include "configuration.hpp"

#import <GameController/GameController.h>

namespace xs::input::internal
{
    GCExtendedGamepad* gamepad = nil;
    GCExtendedGamepad* gamepad_prev_frame = nil;
    bool get_button_pressed(GCExtendedGamepad* pad, gamepad_button button);

    GCKeyboardInput* keyboard = nil;
    GCKeyboardInput* keyboard_prev_frame = nil;
    bool get_key_pressed(GCKeyboardInput* keyboardInput, int key);
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
    if(controllers.count > 0)
    {
        GCController* controller = controllers[0];
        gamepad = controller.extendedGamepad.capture;
    }
    
    if(keyboard)
        keyboard_prev_frame = keyboard.capture;
    
    keyboard = nil;
    auto ckeyboard = [GCKeyboard coalescedKeyboard];
    if(ckeyboard != nil)
        keyboard = ckeyboard.keyboardInput.capture;
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

bool xs::input::get_axis_once(gamepad_axis axis, double threshold)
{
    return false;
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
    if(keyboard == nil)
        return false;
    return get_key_pressed(keyboard, key);
}

bool xs::input::get_key_once(int key)
{
    if(keyboard == nil)
        return false;
    bool cur = get_key_pressed(keyboard, key);
    bool prv = get_key_pressed(keyboard_prev_frame, key);
    return cur && !prv;
}

bool xs::input::get_mouse()
{
    return true;
}

double xs::input::get_mouse_wheel()
{
	return 0.0;
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

void xs::input::set_gamepad_vibration(double low, double high, double time)
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

bool xs::input::internal::get_key_pressed(GCKeyboardInput* keyboardInput, int key)
{
    GCKeyCode keyCode = 0;

    // Convert from xs key (GLFW_KEY_SOMETHING) to GCKeyCode
    switch(key)
    {
        case 32: keyCode = GCKeyCodeSpacebar; break;
        case 65: keyCode = GCKeyCodeKeyA; break;
        case 66: keyCode = GCKeyCodeKeyB; break;
        case 67: keyCode = GCKeyCodeKeyC; break;
        case 68: keyCode = GCKeyCodeKeyD; break;
        case 69: keyCode = GCKeyCodeKeyE; break;
        case 70: keyCode = GCKeyCodeKeyF; break;
        case 71: keyCode = GCKeyCodeKeyG; break;
        case 72: keyCode = GCKeyCodeKeyH; break;
        case 73: keyCode = GCKeyCodeKeyI; break;
        case 74: keyCode = GCKeyCodeKeyJ; break;
        case 75: keyCode = GCKeyCodeKeyK; break;
        case 76: keyCode = GCKeyCodeKeyL; break;
        case 77: keyCode = GCKeyCodeKeyM; break;
        case 78: keyCode = GCKeyCodeKeyN; break;
        case 79: keyCode = GCKeyCodeKeyO; break;
        case 80: keyCode = GCKeyCodeKeyP; break;
        case 81: keyCode = GCKeyCodeKeyQ; break;
        case 82: keyCode = GCKeyCodeKeyR; break;
        case 83: keyCode = GCKeyCodeKeyS; break;
        case 84: keyCode = GCKeyCodeKeyT; break;
        case 85: keyCode = GCKeyCodeKeyU; break;
        case 86: keyCode = GCKeyCodeKeyV; break;
        case 87: keyCode = GCKeyCodeKeyW; break;
        case 88: keyCode = GCKeyCodeKeyX; break;
        case 89: keyCode = GCKeyCodeKeyY; break;
        case 90: keyCode = GCKeyCodeKeyZ; break;
        case 92: keyCode = GCKeyCodeBackslash; break;
        case 256: keyCode = GCKeyCodeEscape; break;
        case 258: keyCode = GCKeyCodeTab; break;
        case 260: keyCode = GCKeyCodeInsert; break;
        case 262: keyCode = GCKeyCodeRightArrow; break;
        case 263: keyCode = GCKeyCodeLeftArrow; break;
        case 264: keyCode = GCKeyCodeDownArrow; break;
        case 265: keyCode = GCKeyCodeUpArrow; break;
        case 266: keyCode = GCKeyCodePageUp; break;
        case 267: keyCode = GCKeyCodePageDown; break;
        case 268: keyCode = GCKeyCodeHome; break;
        case 269: keyCode = GCKeyCodeEnd; break;
        case 280: keyCode = GCKeyCodeCapsLock; break;
        case 281: keyCode = GCKeyCodeScrollLock; break;
    }
    
    GCControllerButtonInput* button = [keyboardInput buttonForKeyCode: keyCode];
    return button.pressed;
}
