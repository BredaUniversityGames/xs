#include <wren.hpp>
#include <cstdint>
#include "input.h"
#include "tools.h"
#include "render.h"
#include "log.h"
#include "script.h"
#include "configuration.h"
#include "registry.h"
#include "fileio.h"

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

void input_get_mousebutton(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int key = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_mousebutton(key);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_mousebutton_once(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const int key = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_mousebutton_once(key);
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
	xs::render::set_color(r, g, b, 1.0);
}

void render_set_color_uint(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto as_double = wrenGetSlotDouble(vm, 1);
	uint32_t as_int = static_cast<uint32_t>(as_double);
	xs::render::color as_color;
	as_color.integer_value = as_int;
	xs::render::set_color(as_color);
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

void render_load_image(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto c_str = wrenGetSlotString(vm, 1);
	const auto str = std::string(c_str);
	auto img = xs::render::load_image(str);
	wrenSetSlotDouble(vm, 0, img);
}

void render_create_sprite(WrenVM* vm)
{
	wrenEnsureSlots(vm, 6);
	const auto id = wrenGetSlotDouble(vm, 1);
	const auto x0 = wrenGetSlotDouble(vm, 2);
	const auto y0 = wrenGetSlotDouble(vm, 3);
	const auto x1 = wrenGetSlotDouble(vm, 4);
	const auto y1 = wrenGetSlotDouble(vm, 5);
	auto sp_id = xs::render::create_sprite((int)id, x0, y0, x1, y1);
	wrenSetSlotDouble(vm, 0, sp_id);
}

void render_sprite(WrenVM* vm)
{
	wrenEnsureSlots(vm, 5);
	const auto sprite_id = wrenGetSlotDouble(vm, 1);
	const auto x = wrenGetSlotDouble(vm, 2);
	const auto y = wrenGetSlotDouble(vm, 3);
	const auto a = wrenGetSlotDouble(vm, 4);
	int a_int = static_cast<int>(a);
	auto a_sa = static_cast<xs::render::sprite_anchor>(a_int);
	xs::render::render_sprite((int)sprite_id, x, y, a_sa);
}

void render_sprite_ex(WrenVM* vm)
{
	wrenEnsureSlots(vm, 5);
	const auto sprite_id = wrenGetSlotDouble(vm, 1);
	const auto x = wrenGetSlotDouble(vm, 2);
	const auto y = wrenGetSlotDouble(vm, 3);
	const auto size = wrenGetSlotDouble(vm, 4);
	const auto rotation = wrenGetSlotDouble(vm, 5);
	const auto mul = wrenGetSlotDouble(vm, 6);
	const auto add = wrenGetSlotDouble(vm, 7);
	const auto flags = wrenGetSlotDouble(vm, 8);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	const auto flags_i = static_cast<uint32_t>(flags);

	xs::render::render_sprite_ex((int)sprite_id, x, y, size, rotation, mul_c, add_c, flags_i);
}

void render_set_offset(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	const auto x = wrenGetSlotDouble(vm, 1);
	const auto y = wrenGetSlotDouble(vm, 2);
	xs::render::set_offset(x, y);
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
// Registry
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
	struct d_color
	{
		double r;
		double g;
		double b;
		double a;
	};
}

void registry_get_number(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::registry::get_number(name, xs::registry::type((int)type));
	wrenSetSlotDouble(vm, 0, value);
}

void registry_get_bool(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::registry::get_bool(name, xs::registry::type((int)type));
	wrenSetSlotBool(vm, 0, value);
}


void registry_get_color(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);	
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::registry::get_color(name, xs::registry::type((int)type));
	wrenSetSlotDouble(vm,0, (double)value);
}

void registry_get_string(WrenVM* vm)
{
}

void registry_set_number(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotDouble(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	xs::registry::set_number(name, val, xs::registry::type((int)type));
}

void registry_set_bool(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotBool(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	xs::registry::set_bool(name, val, xs::registry::type((int)type));
}

void registry_set_color(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotDouble(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	//auto val_uint = (int)
	xs::registry::set_color(name, static_cast<uint32_t>(val), xs::registry::type((int)type));

}

void registry_set_string(WrenVM* vm)
{
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// File
///////////////////////////////////////////////////////////////////////////////////////////////////

void file_read(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto c_src = wrenGetSlotString(vm, 1);
	const auto text = xs::fileio::read_text_file(c_src);
	wrenSetSlotString(vm, 0, text.c_str());
}

void file_write(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	const auto c_text = wrenGetSlotString(vm, 1);
	const auto c_dst = wrenGetSlotString(vm, 2);
	auto success = xs::fileio::write_text_file(c_text, c_dst);
	wrenSetSlotBool(vm, 0, success);
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
	bind("xs", "Input", true, "getMouseButton(_)", input_get_mousebutton);
	bind("xs", "Input", true, "getMouseButtonOnce(_)", input_get_mousebutton_once);

	// Render
	bind("xs", "Render", true, "begin(_)", render_begin);
	bind("xs", "Render", true, "end()", render_end);
	bind("xs", "Render", true, "vertex(_,_)", render_vertex);
	bind("xs", "Render", true, "setColor(_,_,_)", render_set_color);
	bind("xs", "Render", true, "setColor_(_)", render_set_color_uint);
	bind("xs", "Render", true, "text(_,_,_,_)", render_text);
	bind("xs", "Render", true, "line(_,_,_,_)", render_line);
	bind("xs", "Render", true, "loadImage(_)", render_load_image);
	bind("xs", "Render", true, "createSprite(_,_,_,_,_)", render_create_sprite);
	bind("xs", "Render", true, "renderSprite(_,_,_,_)", render_sprite);
	bind("xs", "Render", true, "setOffset(_,_)", render_set_offset);
	bind("xs", "Render", true, "renderSprite(_,_,_,_,_,_,_,_)", render_sprite_ex);

	// Configuration
	bind("xs", "Configuration", true, "title=(_)", configuration_set_title);
	bind("xs", "Configuration", true, "title", configuration_get_title);
	bind("xs", "Configuration", true, "width=(_)", configuration_set_width);
	bind("xs", "Configuration", true, "width", configuration_get_width);
	bind("xs", "Configuration", true, "height=(_)", configuration_set_height);
	bind("xs", "Configuration", true, "height", configuration_get_height);
	bind("xs", "Configuration", true, "multiplier=(_)", configuration_set_multiplier);
	bind("xs", "Configuration", true, "multiplier", configuration_get_multiplier);

	// Registry
	bind("xs", "Registry", true, "getNumber(_,_)", registry_get_number);
	bind("xs", "Registry", true, "getColorNum(_,_)", registry_get_color);
	bind("xs", "Registry", true, "getBool(_,_)", registry_get_bool);
	bind("xs", "Registry", true, "getString(_,_)", registry_get_string);
	bind("xs", "Registry", true, "setNumber(_,_,_)", registry_set_number);
	bind("xs", "Registry", true, "setColorNum(_,_,_)", registry_set_color);
	bind("xs", "Registry", true, "setBool(_,_,_)", registry_set_bool);
	bind("xs", "Registry", true, "setString(_,_,_)", registry_set_string);

	// File
	bind("xs", "File", true, "read(_)", file_read);
	bind("xs", "File", true, "write(_,_)", file_write);
}