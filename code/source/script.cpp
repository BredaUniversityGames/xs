#include <script.h>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <array>
#include <wren.hpp>
#include "fileio.h"
#include "log.h"
#include "profiler.h"
#include "data.h"
#include "input.h"
#include "tools.h"
#include "render.h"
#include "script.h"
#include "configuration.h"
#include "fileio.h"
#include "audio.h"
#include "device.h"

extern "C" {
#include "wren_opt_random.h"
}

using namespace std;

namespace xs::script::internal
{
	WrenVM* vm = nullptr;
	WrenHandle* game_class = nullptr;
	WrenHandle* config_method = nullptr;
	WrenHandle* init_method = nullptr;
	WrenHandle* update_method = nullptr;
	WrenHandle* render_method = nullptr;
	std::unordered_map<size_t, WrenForeignMethodFn> foreign_methods;
	std::unordered_map<size_t, std::string> modules;
	bool initialized = false;
	std::string main;
	std::string main_module;
	bool error = false;
	
	void writeFn(WrenVM* vm, const char* text)
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

	void errorFn(
		WrenVM* vm,
		WrenErrorType errorType,
	    const char* module,
		const int line,
		const char* msg)
	{
	    switch (errorType)
	    {
	    case WREN_ERROR_COMPILE:
	    {
			xs::log::error("[{} line {}] [error] {}", module, line, msg);
	    } break;
	    case WREN_ERROR_STACK_TRACE:
	    {
			xs::log::error("[{} line {}] in {}", module, line, msg);
	    } break;
	    case WREN_ERROR_RUNTIME:
	    {
			xs::log::error("[Runtime Error] {}", msg);
	    } break;
	    }

		error = true;
	}

	size_t get_method_id(
		const std::string& module,
		const std::string& class_name,
		bool is_static,
		const std::string& signature)
	{
		const string dot = is_static ? "::" : ".";
		const string concat = module + "::" + class_name + dot + signature;
		return hash<string>{}(concat);
	}

	WrenForeignMethodFn bindForeignMethod(
		WrenVM* vm,
		const char* module,
		const char* class_name,
		bool is_static,
		const char* signature)
	{
		if (strcmp(module, "random") == 0)
			return wrenRandomBindForeignMethod(vm, class_name, is_static, signature);

		const auto id = get_method_id(module, class_name, is_static, signature);
		const auto itr = foreign_methods.find(id);
		if (itr != foreign_methods.end())
			return itr->second;
		return nullptr;
	}

	WrenForeignClassMethods bindForeignClass(
		WrenVM* vm,
		const char* module,
		const char* className)
	{
		if (strcmp(module, "random") == 0)
			return wrenRandomBindForeignClass(vm, module, className);
		
		WrenForeignClassMethods res{};
		return res;
	}

	WrenLoadModuleResult loadModule(WrenVM* vm, const char* name)
	{
		WrenLoadModuleResult res {};
		if(strcmp(name, "random") == 0)
		{
			res.source = wrenRandomSource();
		}
		else
		{
			auto filename = "[games]/shared/modules/" + string(name) + ".wren";
			if (!xs::fileio::exists(filename))
			{
				auto mstring = string(main);
				auto i = mstring.find_last_of('/');
				filename = mstring.erase(i) + '/' + string(name) + ".wren";
				if (!xs::fileio::exists(filename))
				{
					log::warn("Module '{}' can not be found!", name);
				}
			}
			const auto id = hash<string>{}(filename);
			modules[id] = xs::fileio::read_text_file(filename);
			res.source = modules[id].c_str();
		}
		return res;
	}
}

using namespace xs::script::internal;

void xs::script::configure()
{
	xs::profiler::begin_timing();

	initialized = false;
	error = false;

	internal::main = "[game]/game.wren";

	if (!fileio::exists(internal::main))
	{
		log::error("Wren script file {} not found!", internal::main);
		log::error("Please restart with a valid script file!", internal::main);
		return;
	}
	
	log::info("Wren script set to {}", internal::main);
	bind_api();

    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn = &writeFn;
    config.errorFn = &errorFn;
	config.bindForeignMethodFn = &bindForeignMethod;
	config.bindForeignClassFn = &bindForeignClass;
	config.loadModuleFn = &loadModule;
	//config.initialHeapSize = 1024 * 1024 * 32;
	//config.minHeapSize = 1024 * 1024 * 16;
	//config.heapGrowthPercent = 80;

	vm = wrenNewVM(&config);
	main_module = string(internal::main);
	auto l_slash = main_module.find_last_of("/");
	main_module.erase(0, l_slash + 1);
	auto l_dot = main_module.find_last_of(".");
	main_module.erase(l_dot, main_module.length());

	const std::string script_file = fileio::read_text_file(internal::main);
	const WrenInterpretResult result = wrenInterpret(vm, main_module.c_str(), script_file.c_str());
	
	switch (result)
	{
	case WREN_RESULT_COMPILE_ERROR:
		{
			log::error("Compile Error!");
			error = true;
		} break;
	case WREN_RESULT_RUNTIME_ERROR:
		{
		log::error("Runtime Error!");
			error = true;
		} break;
	case WREN_RESULT_SUCCESS:
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
			log::info(string("Game compile success. ") + pr);
		} break;
	}

	if (initialized && !error)
	{
		wrenEnsureSlots(vm, 1);										// Make sure there at least one slot
		wrenGetVariable(vm, main_module.c_str(), "Game", 0);		// Grab a handle to the Game class
		game_class = wrenGetSlotHandle(vm, 0);
		wrenSetSlotHandle(vm, 0, game_class);						// Put Game class in slot 0
		config_method = wrenMakeCallHandle(vm, "config()");
		init_method = wrenMakeCallHandle(vm, "init()");
		update_method = wrenMakeCallHandle(vm, "update(_)");
		render_method = wrenMakeCallHandle(vm, "render()");
		wrenCall(vm, config_method);
	}

	auto timing = xs::profiler::end_timing();
	log::info("Game compile and configure took {} ms.", timing);
	
}

void xs::script::initialize()
{
	if (initialized)
	{
		wrenEnsureSlots(vm, 1);
		wrenSetSlotHandle(vm, 0, game_class);
		wrenCall(vm, init_method);
	}
}

void xs::script::shutdown()
{
	if (vm)
	{		
		if (game_class)
		{
			wrenReleaseHandle(vm, game_class);
			game_class = nullptr;
		}
		if (config_method)
		{
			wrenReleaseHandle(vm, config_method);
			config_method = nullptr;
		}
		if (init_method)
		{
			wrenReleaseHandle(vm, init_method);
			init_method = nullptr;
		}
		if (update_method)
		{
			wrenReleaseHandle(vm, update_method);
			update_method = nullptr;
		}
		if (render_method)
		{
			wrenReleaseHandle(vm, render_method);
			render_method = nullptr;
		}
		wrenFreeVM(vm);
		vm = nullptr;
	}

	foreign_methods.clear();
	modules.clear();
}

void xs::script::update(double dt)
{
	XS_PROFILE_FUNCTION();
	if (initialized)
	{
		wrenEnsureSlots(vm, 2);
		wrenSetSlotHandle(vm, 0, game_class);
		wrenSetSlotDouble(vm, 1, dt);
		wrenCall(vm, update_method);
	}
}

void xs::script::render()
{
	XS_PROFILE_FUNCTION();
	if (initialized)
	{
		wrenEnsureSlots(vm, 1);
		wrenSetSlotHandle(vm, 0, game_class);
		wrenCall(vm, render_method);
	}
}

bool xs::script::has_error()
{
	return error;
}

void xs::script::clear_error()
{
	error = false;
}

void xs::script::bind(
	const std::string& module,
	const std::string& class_name,
	bool is_static,
	const std::string& signature,
	WrenForeignMethodFn func)
{
	const auto id = get_method_id(module, class_name, is_static, signature);
	foreign_methods[id] = func;
}


 
///////////////////////////////////////////////////////////////////////////////////////////////////
// 
//										All xs API
// 
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define CHECK_TYPE(vm, slot, type) if(!checkType((vm), (slot), (type), (__func__))) { \
			xs::script::internal::error = true; \
			wrenEnsureSlots(vm, 1); \
			wrenSetSlotBool(vm, 0, false); \
			wrenAbortFiber(xs::script::internal::vm, 0); \
			return; }
#else
#define CHECK_TYPE(vm, slot, type) 
#endif

std::string get_type_name(WrenType type)
{
	switch (type)
	{
	case WREN_TYPE_BOOL:
		return "bool";
	case WREN_TYPE_NUM:
		return "number";
	case WREN_TYPE_FOREIGN:
		return "foregin";
	case WREN_TYPE_LIST:
		return "list";
	case WREN_TYPE_MAP:
		return "map";
	case WREN_TYPE_NULL:
		return "null";
	case  WREN_TYPE_STRING:
		return "string";
	case WREN_TYPE_UNKNOWN:
		return "unknown";
	default:
		return "default";
	}
}

bool checkType(WrenVM* vm, int slot, WrenType type, const std::string& function)
{
	if (wrenGetSlotType(vm, slot) == type)
		return true;

	auto ex = get_type_name(type);
	auto rc = get_type_name(wrenGetSlotType(vm, slot));
	xs::log::error("Invalid type passed in function '{}' in slot '{}'", function, slot);
	xs::log::error("Expected '{}', recieved '{}'", ex, rc);
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Input
///////////////////////////////////////////////////////////////////////////////////////////////////

void input_get_axis(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto axis = xs::input::gamepad_axis(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_axis(axis);
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_button(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto button = xs::input::gamepad_button(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_button(button);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_button_once(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto button = xs::input::gamepad_button(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_button_once(button);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_key(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const int key = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_key(key);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_key_once(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const int key = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_key_once(key);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_mouse(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::input::get_mouse();
	wrenSetSlotBool(vm, 0, output);
}

void input_get_mousebutton(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto button = xs::input::mouse_button(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_mousebutton(button);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_mousebutton_once(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto button = xs::input::mouse_button(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_mousebutton_once(button);
	wrenSetSlotBool(vm, 0, output);
}

void input_get_mouse_x(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::input::get_mouse_x();
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_mouse_y(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::input::get_mouse_y();
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_nr_touches(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::input::get_nr_touches();
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_touch_id(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const int index = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_touch_id(index);
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_touch_x(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const int index = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_touch_x(index);
	wrenSetSlotDouble(vm, 0, output);
}

void input_get_touch_y(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const int index = static_cast<int>(wrenGetSlotDouble(vm, 1));
	const auto output = xs::input::get_touch_y(index);
	wrenSetSlotDouble(vm, 0, output);
}

void input_set_gamepad_vibration(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);

	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);

	const auto leftRumble = wrenGetSlotDouble(vm, 1);
	const auto rightRumble = wrenGetSlotDouble(vm, 2);

	xs::input::set_gamepad_vibration(static_cast<int>(leftRumble), static_cast<int>(rightRumble));
}

void input_set_lightbar_color(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);

	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);

	const auto r = wrenGetSlotDouble(vm, 1);
	const auto g = wrenGetSlotDouble(vm, 2);
	const auto b = wrenGetSlotDouble(vm, 3);

	xs::input::set_lightbar_color(r,g,b);
}

void input_reset_lightbar(WrenVM* vm)
{
	xs::input::reset_lightbar();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////////////////////////

void render_begin(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto p = wrenGetSlotDouble(vm, 1);
	if (p == 1.0)
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
	wrenEnsureSlots(vm, 3);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	const auto x = wrenGetSlotDouble(vm, 1);
	const auto y = wrenGetSlotDouble(vm, 2);
	xs::render::vertex(x, y);
}

void render_set_color(WrenVM* vm)
{
	wrenEnsureSlots(vm, 5);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 4, WREN_TYPE_NUM);
	const auto r = wrenGetSlotDouble(vm, 1);
	const auto g = wrenGetSlotDouble(vm, 2);
	const auto b = wrenGetSlotDouble(vm, 3);
	const auto a = wrenGetSlotDouble(vm, 4);
	xs::render::set_color(r, g, b, a);
}

void render_set_color_uint(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto as_double = wrenGetSlotDouble(vm, 1);
	uint32_t as_int = static_cast<uint32_t>(as_double);
	xs::render::color as_color;
	as_color.integer_value = as_int;
	xs::render::set_color(as_color);
}

void render_line(WrenVM* vm)
{
	wrenEnsureSlots(vm, 5);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 4, WREN_TYPE_NUM);
	const auto x0 = wrenGetSlotDouble(vm, 1);
	const auto y0 = wrenGetSlotDouble(vm, 2);
	const auto x1 = wrenGetSlotDouble(vm, 3);
	const auto y1 = wrenGetSlotDouble(vm, 4);
	xs::render::line(x0, y0, x1, y1);
}

void render_text(WrenVM* vm)
{
	wrenEnsureSlots(vm, 5);
	CHECK_TYPE(vm, 1, WREN_TYPE_STRING);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 4, WREN_TYPE_NUM);
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

void render_get_image_width(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	int id = (int)wrenGetSlotDouble(vm, 1);
	int width = xs::render::get_image_width(id);
	wrenSetSlotDouble(vm, 0, width);
}


void render_get_image_height(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	int id = (int)wrenGetSlotDouble(vm, 1);
	int height = xs::render::get_image_height(id);
	wrenSetSlotDouble(vm, 0, height);
}

void render_create_sprite(WrenVM* vm)
{
	wrenEnsureSlots(vm, 6);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 4, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 5, WREN_TYPE_NUM);
	const auto id = wrenGetSlotDouble(vm, 1);
	const auto x0 = wrenGetSlotDouble(vm, 2);
	const auto y0 = wrenGetSlotDouble(vm, 3);
	const auto x1 = wrenGetSlotDouble(vm, 4);
	const auto y1 = wrenGetSlotDouble(vm, 5);
	auto sp_id = xs::render::create_sprite((int)id, x0, y0, x1, y1);
	wrenSetSlotDouble(vm, 0, sp_id);
}

void render_sprite_ex(WrenVM* vm)
{
	wrenEnsureSlots(vm, 10);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 4, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 5, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 6, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 7, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 8, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 9, WREN_TYPE_NUM);
	const auto sprite_id = wrenGetSlotDouble(vm, 1);
	const auto x = wrenGetSlotDouble(vm, 2);
	const auto y = wrenGetSlotDouble(vm, 3);
	const auto z = wrenGetSlotDouble(vm, 4);
	const auto scale = wrenGetSlotDouble(vm, 5);
	const auto rotation = wrenGetSlotDouble(vm, 6);
	const auto mul = wrenGetSlotDouble(vm, 7);
	const auto add = wrenGetSlotDouble(vm, 8);
	const auto flags = wrenGetSlotDouble(vm, 9);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	const auto flags_i = static_cast<uint32_t>(flags);

	xs::render::render_sprite((int)sprite_id, x, y, z, scale, rotation, mul_c, add_c, flags_i);
}

void render_set_offset(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	const auto x = wrenGetSlotDouble(vm, 1);
	const auto y = wrenGetSlotDouble(vm, 2);
	xs::render::set_offset(x, y);
}

void render_load_font(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	CHECK_TYPE(vm, 1, WREN_TYPE_STRING);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);

	const auto c_str = wrenGetSlotString(vm, 1);
	const auto size = wrenGetSlotDouble(vm, 2);
	const auto str = std::string(c_str);
	auto font = xs::render::load_font(str, size);
	wrenSetSlotDouble(vm, 0, font);
}

void render_render_text(WrenVM* vm)
{
	wrenEnsureSlots(vm, 8);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_STRING);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 4, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 5, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 6, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 7, WREN_TYPE_NUM);

	const auto font_id = wrenGetSlotDouble(vm, 1);
	const auto c_text = wrenGetSlotString(vm, 2);
	const auto x = wrenGetSlotDouble(vm, 3);
	const auto y = wrenGetSlotDouble(vm, 4);
	const auto mul = wrenGetSlotDouble(vm, 5);
	const auto add = wrenGetSlotDouble(vm, 6);
	const auto flags = wrenGetSlotDouble(vm, 7);

	std::string	text(c_text);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	const auto flags_i = static_cast<uint32_t>(flags);

	render_text((int)font_id, c_text, (float)x, (float)y, mul_c, add_c, flags_i);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Audio
///////////////////////////////////////////////////////////////////////////////////////////////////

void audio_load(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	CHECK_TYPE(vm, 1, WREN_TYPE_STRING);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	const auto filename = wrenGetSlotString(vm, 1);
	const auto group_id = wrenGetSlotDouble(vm, 2);
	auto sound_id = xs::audio::load(filename, static_cast<int>(group_id));
	wrenSetSlotDouble(vm, 0, sound_id);
}

void audio_play(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto sound_id = wrenGetSlotDouble(vm, 1);
	int channel_id = xs::audio::play(static_cast<int>(sound_id));
	wrenSetSlotDouble(vm, 0, channel_id);
}

void audio_get_group_volume(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto group_id = wrenGetSlotDouble(vm, 1);
	const auto volume = xs::audio::get_group_volume(static_cast<int>(group_id));
	wrenSetSlotDouble(vm, 0, volume);
}

void audio_set_group_volume(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	const auto group_id = wrenGetSlotDouble(vm, 1);
	const auto volume = wrenGetSlotDouble(vm, 2);
	xs::audio::set_group_volume(static_cast<int>(group_id), volume);
}

void audio_get_channel_volume(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto channel_id = wrenGetSlotDouble(vm, 1);
	const auto volume = xs::audio::get_channel_volume(static_cast<int>(channel_id));
	wrenSetSlotDouble(vm, 0, volume);
}

void audio_set_channel_volume(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_NUM);
	const auto channel_id = wrenGetSlotDouble(vm, 1);
	const auto volume = wrenGetSlotDouble(vm, 2);
	xs::audio::set_channel_volume(static_cast<int>(channel_id), volume);
}

void audio_load_bank(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_STRING);
	const auto filename = wrenGetSlotString(vm, 1);
	xs::audio::load_bank(filename);
}

void audio_unload_bank(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	const auto bank_id = wrenGetSlotDouble(vm, 1);
	xs::audio::unload_bank(static_cast<int>(bank_id));
}

void audio_start_event(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	CHECK_TYPE(vm, 1, WREN_TYPE_STRING);
	const auto eventname = wrenGetSlotString(vm, 1);
	xs::audio::start_event(eventname);
}

void audio_set_parameter_number(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_STRING);
	CHECK_TYPE(vm, 3, WREN_TYPE_NUM);
	const auto event_id = wrenGetSlotDouble(vm, 1);
	const auto paramname = wrenGetSlotString(vm, 2);
	const auto value = wrenGetSlotDouble(vm, 3);
	xs::audio::set_parameter_number(static_cast<int>(event_id), paramname, value);
}

void audio_set_parameter_label(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	CHECK_TYPE(vm, 1, WREN_TYPE_NUM);
	CHECK_TYPE(vm, 2, WREN_TYPE_STRING);
	CHECK_TYPE(vm, 3, WREN_TYPE_STRING);
	const auto event_id = wrenGetSlotDouble(vm, 1);
	const auto paramname = wrenGetSlotString(vm, 2);
	const auto value = wrenGetSlotString(vm, 3);
	xs::audio::set_parameter_label(static_cast<int>(event_id), paramname, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Data
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

void data_get_number(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::data::get_number(name, xs::data::type((int)type));
	wrenSetSlotDouble(vm, 0, value);
}

void data_get_bool(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::data::get_bool(name, xs::data::type((int)type));
	wrenSetSlotBool(vm, 0, value);
}


void data_get_color(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::data::get_color(name, xs::data::type((int)type));
	wrenSetSlotDouble(vm, 0, (double)value);
}

void data_get_string(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto name = wrenGetSlotString(vm, 1);
	const auto type = wrenGetSlotDouble(vm, 2);
	auto value = xs::data::get_string(name, xs::data::type((int)type));
	wrenSetSlotString(vm, 0, value.c_str());
}

void data_set_number(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotDouble(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	xs::data::set_number(name, val, xs::data::type((int)type));
}

void data_set_bool(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotBool(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	xs::data::set_bool(name, val, xs::data::type((int)type));
}

void data_set_color(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotDouble(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	//auto val_uint = (int)
	xs::data::set_color(name, static_cast<uint32_t>(val), xs::data::type((int)type));

}

void data_set_string(WrenVM* vm)
{
	wrenEnsureSlots(vm, 4);
	auto name = wrenGetSlotString(vm, 1);
	auto val = wrenGetSlotString(vm, 2);
	auto type = wrenGetSlotDouble(vm, 3);
	xs::data::set_string(name, val, xs::data::type((int)type));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// File
///////////////////////////////////////////////////////////////////////////////////////////////////

void file_read(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
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

void file_exists(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);
	const auto c_text = wrenGetSlotString(vm, 1);
	auto success = xs::fileio::exists(c_text);
	wrenSetSlotBool(vm, 0, success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Device
///////////////////////////////////////////////////////////////////////////////////////////////////

void device_get_platform(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::device::get_platform();
	wrenSetSlotDouble(vm, 0, static_cast<double>(output));
}

void device_can_close(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::device::can_close();
	wrenSetSlotDouble(vm, 0, output);
}

void device_request_close(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	const auto output = xs::device::request_close();
	wrenSetSlotBool(vm, 0, output);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// 
//											Bind xs API
// 
///////////////////////////////////////////////////////////////////////////////////////////////////
void xs::script::bind_api()
{
	// Input
	bind("xs", "Input", true, "getAxis(_)", input_get_axis);
	bind("xs", "Input", true, "getButton(_)", input_get_button);
	bind("xs", "Input", true, "getButtonOnce(_)", input_get_button_once);
	bind("xs", "Input", true, "getKey(_)", input_get_key);
	bind("xs", "Input", true, "getKeyOnce(_)", input_get_key_once);
	bind("xs", "Input", true, "getMouse()", input_get_mouse);
	bind("xs", "Input", true, "getMouseButton(_)", input_get_mousebutton);
	bind("xs", "Input", true, "getMouseButtonOnce(_)", input_get_mousebutton_once);
	bind("xs", "Input", true, "getMouseX()", input_get_mouse_x);
	bind("xs", "Input", true, "getMouseY()", input_get_mouse_y);
	bind("xs", "Input", true, "getNrTouches()", input_get_nr_touches);
	bind("xs", "Input", true, "getTouchId(_)", input_get_touch_id);
	bind("xs", "Input", true, "getTouchX(_)", input_get_touch_x);
	bind("xs", "Input", true, "getTouchY(_)", input_get_touch_y);
	bind("xs", "Input", true, "setPadVibration(_,_)", input_set_gamepad_vibration);
	bind("xs", "Input", true, "setPadLightbarColor(_,_,_)", input_set_lightbar_color);
	bind("xs", "Input", true, "resetPadLightbarColor()", input_reset_lightbar);

	// Render
	bind("xs", "Render", true, "begin(_)", render_begin);
	bind("xs", "Render", true, "end()", render_end);
	bind("xs", "Render", true, "vertex(_,_)", render_vertex);
	bind("xs", "Render", true, "setColor(_,_,_,_)", render_set_color);
	bind("xs", "Render", true, "setColor(_)", render_set_color_uint);
	bind("xs", "Render", true, "shapeText(_,_,_,_)", render_text);
	bind("xs", "Render", true, "line(_,_,_,_)", render_line);
	bind("xs", "Render", true, "loadImage(_)", render_load_image);
	bind("xs", "Render", true, "getImageWidth(_)", render_get_image_width);
	bind("xs", "Render", true, "getImageHeight(_)", render_get_image_height);
	bind("xs", "Render", true, "createSprite(_,_,_,_,_)", render_create_sprite);
	bind("xs", "Render", true, "setOffset(_,_)", render_set_offset);
	bind("xs", "Render", true, "sprite(_,_,_,_,_,_,_,_,_)", render_sprite_ex);
	bind("xs", "Render", true, "loadFont(_,_)", render_load_font);
	bind("xs", "Render", true, "text(_,_,_,_,_,_,_)", render_render_text);

	// Audio
	bind("xs", "Audio", true, "load(_,_)", audio_load);
	bind("xs", "Audio", true, "play(_)", audio_play);
	bind("xs", "Audio", true, "getGroupVolume(_)", audio_get_group_volume);
	bind("xs", "Audio", true, "setGroupVolume(_,_)", audio_set_group_volume);
	bind("xs", "Audio", true, "getChannelVolume(_)", audio_get_channel_volume);
	bind("xs", "Audio", true, "setChannelVolume(_,_)", audio_set_channel_volume);

	// Data
	bind("xs", "Data", true, "getNumber(_,_)", data_get_number);
	bind("xs", "Data", true, "getColor(_,_)", data_get_color);
	bind("xs", "Data", true, "getBool(_,_)", data_get_bool);
	bind("xs", "Data", true, "getString(_,_)", data_get_string);
	bind("xs", "Data", true, "setNumber(_,_,_)", data_set_number);
	bind("xs", "Data", true, "setColor(_,_,_)", data_set_color);
	bind("xs", "Data", true, "setBool(_,_,_)", data_set_bool);
	bind("xs", "Data", true, "setString(_,_,_)", data_set_string);

	// File
	bind("xs", "File", true, "read(_)", file_read);
	bind("xs", "File", true, "write(_,_)", file_write);
	bind("xs", "File", true, "exists(_)", file_exists);

	// Device
	bind("xs", "Device", true, "getPlatform()", device_get_platform);
	bind("xs", "Device", true, "canClose()", device_can_close);
	bind("xs", "Device", true, "requestClose()", device_request_close);
}
