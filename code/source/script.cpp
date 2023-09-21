#include <script.h>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <functional>
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
	std::unordered_map<size_t, string> modules;
	bool initialized = false;
	string main;
	string main_module;
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
		const string& module,
		const string& class_name,
		bool is_static,
		const string& signature)
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

	const string& script_file = fileio::read_text_file(internal::main);
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
	const string& module,
	const string& class_name,
	bool is_static,
	const string& signature,
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

string get_type_name(WrenType type)
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

bool checkType(WrenVM* vm, int slot, WrenType type, const string& function)
{
	if (wrenGetSlotType(vm, slot) == type)
		return true;

	auto ex = get_type_name(type);
	auto rc = get_type_name(wrenGetSlotType(vm, slot));
	xs::log::error("Invalid type passed in function '{}' in slot '{}'", function, slot);
	xs::log::error("Expected '{}', recieved '{}'", ex, rc);
	return false;
}

template <typename T> T wrenGetParameter(WrenVM* vm, int slot)
{
	if (checkType(vm, slot, WREN_TYPE_NUM, __func__))
		return (T)wrenGetSlotDouble(vm, slot);
	return (T)0;
}

template<> bool wrenGetParameter<bool>(WrenVM* vm, int slot)
{
	if (checkType(vm, slot, WREN_TYPE_BOOL, __func__))
		return wrenGetSlotBool(vm, slot);
	return false;
}
template<> double wrenGetParameter<double>(WrenVM* vm, int slot)
{
	if (checkType(vm, slot, WREN_TYPE_NUM, __func__))
		return wrenGetSlotDouble(vm, slot);
	return -1;
}
template<> string wrenGetParameter<string>(WrenVM* vm, int slot)
{
	if (checkType(vm, slot, WREN_TYPE_STRING, __func__))
		return wrenGetSlotString(vm, slot);
	return "";
}
template<> xs::data::type wrenGetParameter<xs::data::type>(WrenVM* vm, int slot)
{
	if (checkType(vm, slot, WREN_TYPE_NUM, __func__))
		return xs::data::type((int)wrenGetSlotDouble(vm, slot));
	return xs::data::type::none;
}

template <typename T> void wrenSetReturnValue(WrenVM* vm, const T& value)
{
	wrenSetSlotDouble(vm, 0, (double)value);
}
template<> void wrenSetReturnValue<bool>(WrenVM* vm, const bool& value)
{
	wrenSetSlotBool(vm, 0, value);
}
template<> void wrenSetReturnValue<double>(WrenVM* vm, const double& value)
{
	wrenSetSlotDouble(vm, 0, value);
}
template<> void wrenSetReturnValue<string>(WrenVM* vm, const string& value)
{
	wrenSetSlotString(vm, 0, value.c_str());
}

template <typename T>
void bindFunction_args(WrenVM* vm, std::function<void(T)> func)
{
	wrenEnsureSlots(vm, 2);
	func(wrenGetParameter<T>(vm, 1));
}

template <typename ReturnType>
void bindFunction_returnType(WrenVM* vm, std::function<ReturnType(void)> func)
{
	wrenEnsureSlots(vm, 1);
	wrenSetReturnValue<ReturnType>(vm, func());
}

template <typename ReturnType, typename T>
void bindFunction_returnType_args(WrenVM* vm, std::function<ReturnType(T)> func)
{
	wrenEnsureSlots(vm, 2);
	auto value = wrenGetParameter<T>(vm, 1);
	wrenSetReturnValue<ReturnType>(vm, func(value));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Input
///////////////////////////////////////////////////////////////////////////////////////////////////

void input_get_axis(WrenVM* vm)
{
	bindFunction_returnType_args<double, xs::input::gamepad_axis>(vm, xs::input::get_axis);
}

void input_get_button(WrenVM* vm)
{
	bindFunction_returnType_args<bool, xs::input::gamepad_button>(vm, xs::input::get_button);
}

void input_get_button_once(WrenVM* vm)
{
	bindFunction_returnType_args<bool, xs::input::gamepad_button>(vm, xs::input::get_button_once);
}

void input_get_key(WrenVM* vm)
{
	bindFunction_returnType_args<bool, int>(vm, xs::input::get_key);
}

void input_get_key_once(WrenVM* vm)
{
	bindFunction_returnType_args<bool, int>(vm, xs::input::get_key_once);
}

void input_get_mouse(WrenVM* vm)
{
	bindFunction_returnType<bool>(vm, xs::input::get_mouse);
}

void input_get_mousebutton(WrenVM* vm)
{
	bindFunction_returnType_args<bool, xs::input::mouse_button>(vm, xs::input::get_mousebutton);
}

void input_get_mousebutton_once(WrenVM* vm)
{
	bindFunction_returnType_args<bool, xs::input::mouse_button>(vm, xs::input::get_mousebutton_once);
}

void input_get_mouse_x(WrenVM* vm)
{
	bindFunction_returnType<double>(vm, xs::input::get_mouse_x);
}

void input_get_mouse_y(WrenVM* vm)
{
	bindFunction_returnType<double>(vm, xs::input::get_mouse_y);
}

void input_get_nr_touches(WrenVM* vm)
{
	bindFunction_returnType<int>(vm, xs::input::get_nr_touches);
}

void input_get_touch_id(WrenVM* vm)
{
	bindFunction_returnType_args<int, int>(vm, xs::input::get_touch_id);
}

void input_get_touch_x(WrenVM* vm)
{
	bindFunction_returnType_args<double, int>(vm, xs::input::get_touch_x);
}

void input_get_touch_y(WrenVM* vm)
{
	bindFunction_returnType_args<double, int>(vm, xs::input::get_touch_y);
}

void input_set_gamepad_vibration(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int,int>(vm, xs::input::set_gamepad_vibration);
	wrenEnsureSlots(vm, 3);

	const auto leftRumble = wrenGetParameter<int>(vm, 1);
	const auto rightRumble = wrenGetParameter<int>(vm, 2);

	xs::input::set_gamepad_vibration(leftRumble, rightRumble);
}

void input_set_lightbar_color(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<double,double,double>(vm, xs::input::set_lightbar_color);
	wrenEnsureSlots(vm, 4);

	const auto r = wrenGetParameter<double>(vm, 1);
	const auto g = wrenGetParameter<double>(vm, 2);
	const auto b = wrenGetParameter<double>(vm, 3);

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
	// TODO: convert to templated function, currently too difficult due to if/else
	wrenEnsureSlots(vm, 2);
	const auto p = wrenGetParameter<int>(vm, 1);
	if (p == 1)
		begin(xs::render::primitive::triangles);
	else if (p == 2)
		begin(xs::render::primitive::lines);
}

void render_end(WrenVM* vm)
{
	xs::render::end();
}

void render_vertex(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<double,double>(vm, xs::render::vertex);
	wrenEnsureSlots(vm, 3);
	const auto x = wrenGetParameter<double>(vm, 1);
	const auto y = wrenGetParameter<double>(vm, 2);
	xs::render::vertex(x, y);
}

void render_set_color(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<double,double,double,double>(vm, xs::render::set_color);
	wrenEnsureSlots(vm, 5);
	const auto r = wrenGetParameter<double>(vm, 1);
	const auto g = wrenGetParameter<double>(vm, 2);
	const auto b = wrenGetParameter<double>(vm, 3);
	const auto a = wrenGetParameter<double>(vm, 4);
	xs::render::set_color(r, g, b, a);
}

void render_set_color_uint(WrenVM* vm)
{
	// TODO: convert to templated function, currently difficult due to extra steps
	wrenEnsureSlots(vm, 2);
	const auto color_int = wrenGetParameter<uint32_t>(vm, 1);
	xs::render::color as_color;
	as_color.integer_value = color_int;
	xs::render::set_color(as_color);
}

void render_line(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<double,double,double,double>(vm, xs::render::line);
	wrenEnsureSlots(vm, 5);
	const auto x0 = wrenGetParameter<double>(vm, 1);
	const auto y0 = wrenGetParameter<double>(vm, 2);
	const auto x1 = wrenGetParameter<double>(vm, 3);
	const auto y1 = wrenGetParameter<double>(vm, 4);
	xs::render::line(x0, y0, x1, y1);
}

void render_text(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<string,double,double,double>(vm, xs::render::text);
	wrenEnsureSlots(vm, 5);
	const auto text = wrenGetParameter<string>(vm, 1);
	const auto x = wrenGetParameter<double>(vm, 2);
	const auto y = wrenGetParameter<double>(vm, 3);
	const auto size = wrenGetParameter<double>(vm, 4);
	xs::render::text(text, x, y, size);
}

void render_load_image(WrenVM* vm)
{
	bindFunction_returnType_args<int, string>(vm, xs::render::load_image);
}

void render_get_image_width(WrenVM* vm)
{
	bindFunction_returnType_args<int, int>(vm, xs::render::get_image_width);
}

void render_get_image_height(WrenVM* vm)
{
	bindFunction_returnType_args<int, int>(vm, xs::render::get_image_height);
}

void render_create_sprite(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int,double,double,double,double>(vm, xs::render::create_sprite);
	wrenEnsureSlots(vm, 6);
	const auto id = wrenGetParameter<int>(vm, 1);
	const auto x0 = wrenGetParameter<double>(vm, 2);
	const auto y0 = wrenGetParameter<double>(vm, 3);
	const auto x1 = wrenGetParameter<double>(vm, 4);
	const auto y1 = wrenGetParameter<double>(vm, 5);
	auto sp_id = xs::render::create_sprite(id, x0, y0, x1, y1);
	wrenSetReturnValue<double>(vm, sp_id);
}

void render_sprite_ex(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int,double,double,double,double,double,double,double,double,double>(vm, xs::render::render_sprite);
	wrenEnsureSlots(vm, 10);
	const auto sprite_id = wrenGetParameter<int>(vm, 1);
	const auto x = wrenGetParameter<double>(vm, 2);
	const auto y = wrenGetParameter<double>(vm, 3);
	const auto z = wrenGetParameter<double>(vm, 4);
	const auto scale = wrenGetParameter<double>(vm, 5);
	const auto rotation = wrenGetParameter<double>(vm, 6);
	const auto mul = wrenGetParameter<double>(vm, 7);
	const auto add = wrenGetParameter<double>(vm, 8);
	const auto flags = wrenGetParameter<uint32_t>(vm, 9);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	xs::render::render_sprite(sprite_id, x, y, z, scale, rotation, mul_c, add_c, flags);
}

void render_set_offset(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<double,double>(vm, xs::render::set_offset);
	wrenEnsureSlots(vm, 3);
	const auto x = wrenGetParameter<double>(vm, 1);
	const auto y = wrenGetParameter<double>(vm, 2);
	xs::render::set_offset(x, y);
}

void render_load_font(WrenVM* vm)
{
	wrenEnsureSlots(vm, 3);
	const auto& str = wrenGetParameter<string>(vm, 1);
	const auto size = wrenGetParameter<double>(vm, 2);
	wrenSetReturnValue(vm, xs::render::load_font(str, size));
}

void render_render_text(WrenVM* vm)
{
	wrenEnsureSlots(vm, 8);
	const auto font_id = wrenGetParameter<int>(vm, 1);
	const auto& text = wrenGetParameter<string>(vm, 2);
	const auto x = wrenGetParameter<double>(vm, 3);
	const auto y = wrenGetParameter<double>(vm, 4);
	const auto mul = wrenGetParameter<double>(vm, 5);
	const auto add = wrenGetParameter<double>(vm, 6);
	const auto flags = wrenGetParameter<uint32_t>(vm, 7);

	xs::render::color mul_c;
	mul_c.integer_value = static_cast<uint32_t>(mul);

	xs::render::color add_c;
	add_c.integer_value = static_cast<uint32_t>(add);

	render_text(font_id, text, x, y, mul_c, add_c, flags);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Audio
///////////////////////////////////////////////////////////////////////////////////////////////////

void audio_load(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_returnType_args<int,string,int>(vm, xs::audio::load);
	wrenEnsureSlots(vm, 3);
	const auto filename = wrenGetParameter<string>(vm, 1);
	const auto group_id = wrenGetParameter<int>(vm, 2);
	wrenSetReturnValue(vm, xs::audio::load(filename, group_id));
}

void audio_play(WrenVM* vm)
{
	bindFunction_returnType_args<int, int>(vm, xs::audio::play);
}

void audio_get_group_volume(WrenVM* vm)
{
	bindFunction_returnType_args<double, int>(vm, xs::audio::get_group_volume);
}

void audio_set_group_volume(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int, double>(vm, xs::audio::set_group_volume);
	wrenEnsureSlots(vm, 3);
	const auto group_id = wrenGetParameter<int>(vm, 1);
	const auto volume = wrenGetParameter<double>(vm, 2);
	xs::audio::set_group_volume(group_id, volume);
}

void audio_get_channel_volume(WrenVM* vm)
{
	bindFunction_returnType_args<double, int>(vm, xs::audio::get_channel_volume);
}

void audio_set_channel_volume(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int, double>(vm, xs::audio::set_channel_volume);
	wrenEnsureSlots(vm, 3);
	const auto channel_id = wrenGetParameter<int>(vm, 1);
	const auto volume = wrenGetParameter<double>(vm, 2);
	xs::audio::set_channel_volume(channel_id, volume);
}

void audio_load_bank(WrenVM* vm)
{
	bindFunction_returnType_args<int, string>(vm, xs::audio::load_bank);
}

void audio_unload_bank(WrenVM* vm)
{
	bindFunction_args<int>(vm, xs::audio::unload_bank);
}

void audio_start_event(WrenVM* vm)
{
	bindFunction_returnType_args<int, string>(vm, xs::audio::start_event);
}

void audio_set_parameter_number(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int, string, double>(vm, xs::audio::set_parameter_number);
	wrenEnsureSlots(vm, 4);
	const auto event_id = wrenGetParameter<int>(vm, 1);
	const auto param_name = wrenGetParameter<string>(vm, 2);
	const auto value = wrenGetParameter<double>(vm, 3);
	xs::audio::set_parameter_number(event_id, param_name, value);
}

void audio_set_parameter_label(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<int, string, string>(vm, xs::audio::set_parameter_label);
	wrenEnsureSlots(vm, 4);
	const auto event_id = wrenGetParameter<int>(vm, 1);
	const auto param_name = wrenGetParameter<string>(vm, 2);
	const auto value = wrenGetParameter<string>(vm, 3);
	xs::audio::set_parameter_label(event_id, param_name, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> void data_get(WrenVM* vm, std::function<T(const string&,xs::data::type)> func)
{
	wrenEnsureSlots(vm, 2);
	const string& name = wrenGetParameter<string>(vm, 1);
	const auto type = wrenGetParameter<xs::data::type>(vm, 2);
	auto value = func(name, type);
	wrenSetReturnValue<T>(vm, value);
}

void data_get_number(WrenVM* vm)
{
	data_get<double>(vm, xs::data::get_number);
}

void data_get_bool(WrenVM* vm)
{
	data_get<bool>(vm, xs::data::get_bool);
}

void data_get_color(WrenVM* vm)
{
	data_get<uint32_t>(vm, xs::data::get_color);
}

void data_get_string(WrenVM* vm)
{
	data_get<string>(vm, xs::data::get_string);
}

template <typename T> void data_set(WrenVM* vm, std::function<void(const string&,T,xs::data::type)> func)
{
	wrenEnsureSlots(vm, 4);
	const string& name = wrenGetParameter<string>(vm, 1);
	const T val = wrenGetParameter<T>(vm, 2);
	const auto type = wrenGetParameter<xs::data::type>(vm, 3);
	func(name, val, type);
}

void data_set_bool(WrenVM* vm)
{
	data_set<bool>(vm, xs::data::set_bool);
}

void data_set_number(WrenVM* vm)
{
	data_set<double>(vm, xs::data::set_number);
}

void data_set_color(WrenVM* vm)
{
	data_set<uint32_t>(vm, xs::data::set_color);
}

void data_set_string(WrenVM* vm)
{
	data_set<string>(vm, xs::data::set_string);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// File
///////////////////////////////////////////////////////////////////////////////////////////////////

void file_read(WrenVM* vm)
{
	bindFunction_returnType_args<string, string>(vm, xs::fileio::read_text_file);
}

void file_write(WrenVM* vm)
{
	// TODO: convert to templated function, multiple arguments
	//bindFunction_args<bool, string, string>(vm, xs::audio::write_text_file);
	wrenEnsureSlots(vm, 3);
	const auto text = wrenGetParameter<string>(vm, 1);
	const auto dst = wrenGetParameter<string>(vm, 2);
	wrenSetReturnValue(vm, xs::fileio::write_text_file(text, dst));
}

void file_exists(WrenVM* vm)
{
	bindFunction_returnType_args<bool, string>(vm, xs::fileio::exists);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Device
///////////////////////////////////////////////////////////////////////////////////////////////////

void device_get_platform(WrenVM* vm)
{
	bindFunction_returnType<xs::device::platform>(vm, xs::device::get_platform);
}

void device_can_close(WrenVM* vm)
{
	bindFunction_returnType<bool>(vm, xs::device::can_close);
}

void device_request_close(WrenVM* vm)
{
	bindFunction_returnType<bool>(vm, xs::device::request_close);
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
	bind("xs", "Audio", true, "loadBank(_)", audio_load_bank);
	bind("xs", "Audio", true, "unloadBank(_)", audio_unload_bank);
	bind("xs", "Audio", true, "startEvent(_)", audio_start_event);
	bind("xs", "Audio", true, "setParameterNumber(_,_,_)", audio_set_parameter_number);
	bind("xs", "Audio", true, "setParameterLabel(_,_,_)", audio_set_parameter_label);

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
