#include <cstdint>
#include "input.h"
#include "tools.h"
#include "render.h"
#include "log.h"
#include "script_lua.h"
#include "configuration.h"
#include "registry.h"
#include "fileio.h"

extern "C"
{
#include "lua/include/lua.h"
#include "lua/include/lauxlib.h"
#include "lua/include/lualib.h"
}




///////////////////////////////////////////////////////////////////////////////////////////////////
// Input
///////////////////////////////////////////////////////////////////////////////////////////////////

// return int = n of results
int input_get_axis(lua_State* L)
{
	const int axis = static_cast<int> (luaL_checknumber(L, 1));
	const auto output = xs::input::get_axis(axis);
	lua_pushnumber(L, output);
	return 1;
}

int input_get_button(lua_State* L)
{
	const int button = static_cast<int> (luaL_checknumber(L, 1));
	const auto output = xs::input::get_button(button);
	lua_pushboolean(L, output);
	return 1;
}

int input_get_button_once(lua_State* L)
{
	const int button = static_cast<int> (luaL_checknumber(L, 1));
	const auto output = xs::input::get_button_once(button);
	lua_pushboolean(L, output);
	return 1;
}

int input_get_key(lua_State* L)
{
	const int key = static_cast<int> (luaL_checknumber(L, 1));
	const auto output = xs::input::get_key(key);
	lua_pushboolean(L, output);
	return 1;
}

int input_get_key_once(lua_State* L)
{
	const int key = static_cast<int> (luaL_checknumber(L, 1));
	const auto output = xs::input::get_key_once(key);
	lua_pushboolean(L, output);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////////////////////////

int render_begin(lua_State* L)
{
	const auto p = luaL_checknumber(L, 1);
	//todo: why float?
	if (p == 1.0)
		begin(xs::render::primitive::triangles);
	else if (p == 2.0)
		begin(xs::render::primitive::lines);
	return 0;
}

int render_end(lua_State* L)
{
	xs::render::end();
	return 0;
}

int render_vertex(lua_State* L)
{
	const auto x = luaL_checknumber(L, 1);
	const auto y = luaL_checknumber(L, 2);
	xs::render::vertex(x, y);
	return 0;
}

int render_set_color(lua_State* L)
{
	const auto r = luaL_checknumber(L, 1);
	const auto g = luaL_checknumber(L, 2);
	const auto b = luaL_checknumber(L, 3);
	xs::render::set_color(r, g, b, 1.0);
	return 0;
}


int render_set_color_uint(lua_State* L)
{
	const auto as_double = luaL_checknumber(L, 1);
	uint32_t as_int = static_cast<uint32_t>(as_double);
	xs::render::color as_color;
	as_color.integer_value = as_int;
	xs::render::set_color(as_color);
	return 0;
}

int render_line(lua_State* L)
{
	const auto x0 = luaL_checknumber(L, 1);
	const auto y0 = luaL_checknumber(L, 2);
	const auto x1 = luaL_checknumber(L, 3);
	const auto y1 = luaL_checknumber(L, 4);
	xs::render::line(x0, y0, x1, y1);
	return 0;
}

int render_text(lua_State* L)
{
	const auto text = luaL_checkstring(L, 1);
	const auto x = luaL_checknumber(L, 2);
	const auto y = luaL_checknumber(L, 3);
	const auto size = luaL_checknumber(L, 4);
	xs::render::text(text, x, y, size);
	return 0;
}

int render_load_image(lua_State* L)
{
	const auto c_str = luaL_checkstring(L, 1);
	const auto str = std::string(c_str);
	auto img = xs::render::load_image(str);
	lua_pushinteger(L, img);
	
	return 1;
}

int render_create_sprite(lua_State* L)
{
	const auto id = luaL_checknumber(L, 1);
	const auto x0 = luaL_checknumber(L, 2);
	const auto y0 = luaL_checknumber(L, 3);
	const auto x1 = luaL_checknumber(L, 4);
	const auto y1 = luaL_checknumber(L, 5);
	auto sp_id = xs::render::create_sprite((int)id, x0, y0, x1, y1);
	lua_pushinteger(L, sp_id);
	return 1;
}

int render_sprite(lua_State* L)
{
	const auto sprite_id = luaL_checknumber(L, 1);
	const auto x = luaL_checknumber(L, 2);
	const auto y = luaL_checknumber(L, 3);
	const auto a = luaL_checknumber(L, 4);
	int a_int = static_cast<int>(a);
	auto a_sa = static_cast<xs::render::sprite_anchor>(a_int);
	xs::render::render_sprite((int)sprite_id, x, y, a_sa);
	return 0;
}

int render_sprite_ex(lua_State* L)
{
	const auto sprite_id = luaL_checknumber(L, 1);
	const auto x = luaL_checknumber(L, 2);
	const auto y = luaL_checknumber(L, 3);
	const auto size = luaL_checknumber(L, 4);
	const auto rotation = luaL_checknumber(L, 5);
	const auto mul = luaL_checknumber(L, 6);
	const auto add = luaL_checknumber(L, 7);
	const auto flags = luaL_checknumber(L, 8);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	const auto flags_i = static_cast<uint32_t>(flags);

	xs::render::render_sprite_ex((int)sprite_id, x, y, size, rotation, mul_c, add_c, flags_i);
	return 0;
}

int render_set_offset(lua_State* L)
{
	const auto x = luaL_checknumber(L, 1);
	const auto y = luaL_checknumber(L, 2);
	xs::render::set_offset(x, y);
	return 0;
}

int render_load_font(lua_State* L)
{
	const auto c_str = luaL_checkstring(L, 1);
	const auto size = luaL_checknumber(L, 2);
	const auto str = std::string(c_str);
	auto font = xs::render::load_font(str, size);
	lua_pushinteger(L, font);
	return 1;
}

int render_render_text(lua_State* L)
{
	const auto font_id = luaL_checknumber(L, 1);
	const auto c_text = luaL_checkstring(L, 2);
	const auto x = luaL_checknumber(L, 3);
	const auto y = luaL_checknumber(L, 4);
	const auto mul = luaL_checknumber(L, 5);
	const auto add = luaL_checknumber(L, 6);
	const auto flags = luaL_checknumber(L, 7);

	std::string	text(c_text);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	const auto flags_i = static_cast<uint32_t>(flags);

	render_text((int)font_id, c_text, (float)x, (float)y, mul_c, add_c, flags_i);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
///////////////////////////////////////////////////////////////////////////////////////////////////

int configuration_get_width(lua_State* L)
{
	lua_pushnumber(L, xs::configuration::width);
	return 1;
}

int configuration_get_height(lua_State* L)
{
	lua_pushnumber(L, xs::configuration::height);
	return 1;
}

int configuration_get_multiplier(lua_State* L)
{
	lua_pushnumber(L, xs::configuration::multiplier);
	return 1;
}

int configuration_get_fullscreen(lua_State* L)
{
	lua_pushnumber(L, xs::configuration::fullscreen);
	return 1;
}

int configuration_get_title(lua_State* L)
{
	lua_pushstring(L, xs::configuration::title.c_str());
	return 1;
}

int configuration_set_width(lua_State* L)
{
	xs::configuration::width = static_cast<int>(luaL_checknumber(L, 1));
	return 0;
}

int configuration_set_height(lua_State* L)
{
	xs::configuration::height = static_cast<int>(luaL_checknumber(L, 1));
	return 0;
}

int configuration_set_multiplier(lua_State* L)
{
	xs::configuration::multiplier = static_cast<int>(luaL_checknumber(L, 1));
	return 0;
}

int configuration_set_fullscreen(lua_State* L)
{
	assert(false);
	return 0;
}

int configuration_set_title(lua_State* L)
{
	const auto str = luaL_checkstring(L, 1);
	xs::configuration::title = std::string(str);
	return 0;
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

int registry_get_number(lua_State* L)
{
	const auto name = luaL_checkstring(L, 1);
	const auto type = luaL_checknumber(L, 2);
	auto value = xs::registry::get_number(name, xs::registry::type((int)type));
	lua_pushnumber(L, value);
	return 1;
}

int registry_get_bool(lua_State* L)
{
	const auto name = luaL_checkstring(L, 1);
	const auto type = luaL_checknumber(L, 2);
	auto value = xs::registry::get_bool(name, xs::registry::type((int)type));
	lua_pushboolean(L, value);
	return 1;
}


int registry_get_color(lua_State* L)
{
	const auto name = luaL_checkstring(L, 1);
	const auto type = luaL_checknumber(L, 2);
	auto value = xs::registry::get_color(name, xs::registry::type((int)type));
	lua_pushnumber(L, (double)value);
	return 1;
}

int registry_get_string(lua_State* L)
{
	return 0;
}

int registry_set_number(lua_State* L)
{
	auto name = luaL_checkstring(L, 1);
	auto val = luaL_checknumber(L, 2);
	auto type = luaL_checknumber(L, 3);
	xs::registry::set_number(name, val, xs::registry::type((int)type));
	return 0;
}

int registry_set_bool(lua_State* L)
{
	auto name = luaL_checkstring(L, 1);
	auto val = lua_toboolean(L, 2);
	auto type = luaL_checknumber(L, 3);
	xs::registry::set_bool(name, val, xs::registry::type((int)type));
	return 0;
}

int registry_set_color(lua_State* L)
{
	auto name = luaL_checkstring(L, 1);
	auto val = luaL_checknumber(L, 2);
	auto type = luaL_checknumber(L, 3);
	//auto val_uint = (int)
	xs::registry::set_color(name, static_cast<uint32_t>(val), xs::registry::type((int)type));
	return 0;

}

int registry_set_string(lua_State* L)
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// File
///////////////////////////////////////////////////////////////////////////////////////////////////

int file_read(lua_State* L)
{
	const auto c_src = luaL_checkstring(L, 1);
	const auto text = xs::fileio::read_text_file(c_src);
	lua_pushstring(L, text.c_str());
	return 1;
}

int file_write(lua_State* L)
{
	const auto c_text = luaL_checkstring(L, 1);
	const auto c_dst = luaL_checkstring(L, 2);
	auto success = xs::fileio::write_text_file(c_text, c_dst);
	lua_pushboolean(L, success);
	return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Bind xs API
///////////////////////////////////////////////////////////////////////////////////////////////////


void xs::script_lua::bind_api(lua_State* L)
{



	// Input
	static const luaL_Reg input_lib[] = {
		{"getAxis",			input_get_axis},
		{"getButton",		input_get_button},
		{"getButtonOnce",	input_get_button_once},
		{"getKey",			input_get_key},
		{"getKeyOnce",		input_get_key_once},
		{NULL, NULL}  /* sentinel */
	};


	luaL_newlib(L, input_lib);
	bind("xs_input", "games/lua/shared/modules/input.lua");

	// Render
	static const struct luaL_Reg render_lib[] = {
		{"begin", render_begin},
		//can't use "end" as function name in lua
		{"rend", render_end},
		{"vertex", render_vertex},
		{"setColor", render_set_color},
		{"setColorUint", render_set_color_uint},
		{"text", render_text},
		{"line", render_line},
		{"loadImage", render_load_image},
		{"createSprite", render_create_sprite},
		{"renderSprite", render_sprite},
		{"setOffset", render_set_offset},
		{"renderSprite", render_sprite_ex},
		{"loadFont", render_load_font},
		{"renderText", render_render_text},
		{NULL, NULL}  /* sentinel */
	};

	luaL_newlib(L, render_lib);
	bind("xs_render", "games/lua/shared/modules/render.lua");


	// Configuration
	static const struct luaL_Reg configuration_lib[] = {
		{"setTitle", configuration_set_title},
		{"getTitle", configuration_get_title},
		{"setWidth", configuration_set_width},
		{"getWidth", configuration_get_width},
		{"setHeight", configuration_set_height},
		{"getHeight", configuration_get_height},
		{"setMultiplier", configuration_set_multiplier},
		{"getMultiplier", configuration_get_multiplier},
		{NULL, NULL}  /* sentinel */
	};
	luaL_newlib(L, configuration_lib);
	bind(
		"xs_configuration", 
		"games/lua/shared/modules/configuration.lua");


	// Registry
	static const struct luaL_Reg registry_lib[] = {
		{"getNumber", registry_get_number    },
		{"getColorNum", registry_get_color   },
		{"getBool", registry_get_bool	    },
		{"getString", registry_get_string    },
		{"setNumber", registry_set_number    },
		{"setColorNum", registry_set_color   },
		{"setBool", registry_set_bool	    },
		{"setString", registry_set_string    },
		{NULL, NULL},  /* sentinel */
	};

	luaL_newlib(L, registry_lib);
	bind(
		"xs_registry", 
		"games/lua/shared/modules/registry.lua");


	// File
	static const struct luaL_Reg file_lib[] = {
		{"read", file_read},
		{"write", file_write},
		{NULL, NULL},  /* sentinel */
	};

	luaL_newlib(L, file_lib);
	bind(
		"xs_file", 
		"games/lua/shared/modules/file.lua");

}