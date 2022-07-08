#pragma once
#include <string>

typedef struct lua_State lua_State;

namespace xs::script_lua
{
	void configure(const std::string& main);
	void initialize();
	void shutdown();
	void update(double dt);
	void render();
	bool has_error();
	void clear_error();
	void bind_api(lua_State* L);
	void bind(
		const char* module_name,
		const char* module_file);
}