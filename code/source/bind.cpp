#include <wren.hpp>
#include "input.h"
#include "tools.h"
#include "render.h"
#include "log.h"
#include "script.h"
#include "configuration.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Input
///////////////////////////////////////////////////////////////////////////////////////////////////

void input_get_axis(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int axis = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_axis(axis);
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_button(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int button = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_button(button);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_button_once(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int button = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_button_once(button);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_key(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int key = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_key(key);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_key_once(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int key = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_key_once(key);
	wrenSetSlotBool(vm, 0, output);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////////////////////////

void render_begin(WrenVM* vm)
{
	const auto p = wrenGetSlotDouble(vm, 1);
	if(p == 1.0)
		begin(xs::render::primitive::triangles);
	else if (p == 2.0)
		begin(xs::render::primitive::lines);
}

void render_end(WrenVM* vm)
{
	xs::render::end();
}

void render_vertex(WrenVM* vm)
{
	const auto x = wrenGetSlotDouble(vm, 1);
	const auto y = wrenGetSlotDouble(vm, 2);
	xs::render::vertex(x, y);
}

void render_set_color(WrenVM* vm)
{
	const auto r = wrenGetSlotDouble(vm, 1);
	const auto g = wrenGetSlotDouble(vm, 2);
	const auto b = wrenGetSlotDouble(vm, 3);
	xs::render::color(r, g, b, 1.0);
}

void render_set_color_hex(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto str = wrenGetSlotString(vm, 1);
	auto color = xs::tools::parse_color(std::string(str));	
	xs::render::color(
		std::get<0>(color),
		std::get<1>(color),
		std::get<2>(color),
		std::get<3>(color));
}

void render_poly(WrenVM* vm)
{
	const auto x = wrenGetSlotDouble(vm, 1);
	const auto y = wrenGetSlotDouble(vm, 2);
	const auto radius = wrenGetSlotDouble(vm, 3);
	const auto sides = static_cast<int>(round(wrenGetSlotDouble(vm, 4)));
	xs::render::poly(x, y, radius, sides);
}

void render_rect(WrenVM* vm)
{
	const auto x = wrenGetSlotDouble(vm, 1);
	const auto y = wrenGetSlotDouble(vm, 2);
	const auto sx = wrenGetSlotDouble(vm, 3);
	const auto sy = wrenGetSlotDouble(vm, 4);
	const auto r = wrenGetSlotDouble(vm, 5);
	xs::render::rect(x, y, sx, sy, r);
}

void render_line(WrenVM* vm)
{
	const auto x0 = wrenGetSlotDouble(vm, 1);
	const auto y0 = wrenGetSlotDouble(vm, 2);
	const auto x1 = wrenGetSlotDouble(vm, 3);
	const auto y1 = wrenGetSlotDouble(vm, 4);
	xs::render::line(x0, y0, x1, y1);
}

void render_text(WrenVM* vm)
{
	wrenEnsureSlots(vm, 5);
	const auto text = wrenGetSlotString(vm, 1);
	const auto x = wrenGetSlotDouble(vm, 2);
	const auto y = wrenGetSlotDouble(vm, 3);
	const auto size = wrenGetSlotDouble(vm, 4);
	xs::render::text(text, x, y, size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
///////////////////////////////////////////////////////////////////////////////////////////////////

void configuration_get_width(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, xs::configuration::width);
}

void configuration_get_height(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, xs::configuration::height);
}

void configuration_get_multiplier(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, xs::configuration::multiplier);
}

void configuration_get_fullscreen(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	wrenSetSlotDouble(vm, 0, xs::configuration::fullscreen);
}

void configuration_get_title(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);	
	wrenSetSlotString(vm, 0, xs::configuration::title.c_str());
}

void configuration_set_width(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	xs::configuration::width = static_cast<int>(wrenGetSlotDouble(vm, 1));
}

void configuration_set_height(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	xs::configuration::height = static_cast<int>(wrenGetSlotDouble(vm, 1));
}

void configuration_set_multiplier(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	xs::configuration::multiplier = static_cast<int>(wrenGetSlotDouble(vm, 1));
}

void configuration_set_fullscreen(WrenVM* vm)
{
	assert(false);
}

void configuration_set_title(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto str = wrenGetSlotString(vm, 1);
	xs::configuration::title = std::string(str);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Bind xs API
///////////////////////////////////////////////////////////////////////////////////////////////////
void xs::script::bind_api()
{	
	// Input
	bind("xs", "Input", true, "getAxis(_)", input_get_axis);
	bind("xs", "Input", true, "getButton(_)", input_get_button);
	bind("xs", "Input", true, "getButtonOnce(_)", input_get_button_once);
	bind("xs", "Input", true, "getKey(_)", input_get_key);
	bind("xs", "Input", true, "getKeyOnce(_)", input_get_key_once);

	// Render
	bind("xs", "Render", true, "begin(_)", render_begin);
	bind("xs", "Render", true, "end()", render_end);
	bind("xs", "Render", true, "vertex(_,_)", render_vertex);
	bind("xs", "Render", true, "setColor(_,_,_)", render_set_color);
	bind("xs", "Render", true, "setColor(_)", render_set_color_hex);
	bind("xs", "Render", true, "text(_,_,_,_)", render_text);
	bind("xs", "Render", true, "polygon(_,_,_,_)", render_poly);
	bind("xs", "Render", true, "rect(_,_,_,_,_)", render_rect);
	bind("xs", "Render", true, "line(_,_,_,_)", render_line);

	// Configuration
	bind("xs", "Configuration", true, "title=(_)", configuration_set_title);
	bind("xs", "Configuration", true, "title", configuration_get_title);
	bind("xs", "Configuration", true, "width=(_)", configuration_set_width);
	bind("xs", "Configuration", true, "width", configuration_get_width);
	bind("xs", "Configuration", true, "height=(_)", configuration_set_height);
	bind("xs", "Configuration", true, "height", configuration_get_height);
	bind("xs", "Configuration", true, "multiplier=(_)", configuration_set_multiplier);
	bind("xs", "Configuration", true, "multiplier", configuration_get_multiplier);
}