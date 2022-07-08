#include <script_lua.h>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <array>
#include "fileio.h"
#include "log.h"
#include "profiler.h"

extern "C"
{
#include "lua/include/lua.h"
#include "lua/include/lauxlib.h"
#include "lua/include/lualib.h"
}

using namespace std;

namespace xs::script_lua::internal
{
	// Create Lua State
	lua_State* state = nullptr;

	const char* game_class = "game";
	const char* config_method = "config";
	const char* init_method = "init";
	const char* update_method = "update";
	const char* render_method = "render";

	bool initialized = false;
	std::string main;
	std::string main_module;
	bool error = false;

	// TODO integrate errors
	// Little error checking utility function
	bool checkLua(lua_State* L, int r)
	{
		if (r != LUA_OK)
		{
			std::string errormsg = lua_tostring(L, -1);
			log::error(errormsg);
			return false;
		}
		return true;
	}

	// TODO:bind to print
	void writeFn(lua_State* L, const char* text)
	{
		if (strcmp(text, "\n") == 0)
			return;

		static auto magenta = "\033[35m";
		static auto reset = "\033[0m";

#if defined(PLATFORM_PC)
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);
		const auto time = std::put_time(&tm, "[%Y-%m-%d %T.%e0] ");
		std::cout << time << "[" << magenta << "script" << reset << "] " << text << endl;
#else
		std::cout << "[" << magenta << "script" << reset << "] " << text << endl;
#endif
	}

	
}

using namespace xs::script_lua::internal;

void xs::script_lua::configure(const std::string& main)
{
	initialized = false;
	error = false;

	if (!main.empty())
		internal::main = main;

	if (!fileio::exists(internal::main))
	{
		log::error("Wren script file {} not found!", internal::main);
		log::error("Please restart with a valid script file!", internal::main);
		return;
	}

	log::info("Wren script set to {}", internal::main);
	

	state = luaL_newstate();
	luaL_openlibs(state);
	bind_api(state);
	main_module = string(internal::main);
	auto l_slash = main_module.find_last_of("/");
	main_module.erase(0, l_slash + 1);
	auto l_dot = main_module.find_last_of(".");
	main_module.erase(l_dot, main_module.length());

	const std::string script_file = fileio::read_text_file(internal::main);
	//TODO use generated path
	//checkLua(state, luaL_dofile(state, "games/lua/examples/hello.lua"));

	int result = luaL_dofile(state, "games/lua/examples/hello.lua");
	// TODO check for different errors and set error and init
	switch (result)
	{
	case LUA_OK:
		{
			initialized = true;
			static int idx = 0;
			static std::array<string, 10> praise =
			{
				"Great!", "Amazing!", "Super!", "You rock!", "You rule!",
				"Nice!", "Sweet!", "Wow!", "You got this!", "Keep it up!"
			};
			string pr = praise[idx];
			idx = (idx + 1) % praise.size();
			log::info(string("Script compile success. ") + pr);
		} break;
	case LUA_ERRRUN:
	{
		log::error("Runtime Error!");
		error = true;
	} break;
	default:
		break;
	}
	
	initialized = true;

	if (initialized && !error)
	{
		lua_getglobal(state, game_class);
		lua_getfield(state, -1, config_method);
		// passing game_class value as self from the stack
		lua_pushvalue(state, -2);
		//todo: check function
		checkLua(state, lua_pcall(state, 1, 0, 0));

		//todo check after loading files
		initialized = true;

	}
}

void xs::script_lua::initialize()
{
	if (initialized)
	{
		lua_getglobal(state, game_class);
		lua_getfield(state, -1, init_method);
		lua_pushvalue(state, -2);
		checkLua(state, lua_pcall(state, 1, 0, 0));
	}
}


void xs::script_lua::shutdown()
{
	//todo 
	lua_close(state);
}

void xs::script_lua::update(double dt)
{
	if (initialized)
	{
		lua_getglobal(state, game_class);
		lua_getfield(state, -1, update_method);
		// passing game_class value as self from the stack
		lua_pushvalue(state, -2);
		lua_pushnumber(state, dt);
		//todo: check function
		checkLua(state, lua_pcall(state, 2, 0, 0));
	}
}

void xs::script_lua::render()
{
	XS_PROFILE_FUNCTION();
	if (initialized)
	{
		lua_getglobal(state, game_class);
		lua_getfield(state, -1, render_method);
		lua_pushvalue(state, -2);
		checkLua(state, lua_pcall(state, 1, 0, 0));
	}
}

bool xs::script_lua::has_error()
{
	return false;
}

void xs::script_lua::clear_error()
{
	error = false;
}


void xs::script_lua::bind(
	const char* module_name,
	const char* module_file)
{
	lua_setglobal(state, module_name);
	//todo check
	checkLua(state, luaL_dofile(state, module_file));
}


