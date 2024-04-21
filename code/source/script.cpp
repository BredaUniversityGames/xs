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
#include "wren/optional/wren_opt_random.h"
#include "wren/optional/wren_opt_meta.h"
#include "wren_vm.h"
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
    std::unordered_map<size_t, WrenForeignClassMethods> foreign_classes;
    std::unordered_map<size_t, string> modules;
    bool initialized = false;
    string main;
    string main_module;
    bool error = false;

    void writeFn(WrenVM* vm, const char* text)
    {
        if (strcmp(text, "\n") == 0)
            return;

#if defined(PLATFORM_PC)
        static auto magenta = "\033[35m";
        static auto reset = "\033[0m";
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        const auto time = std::put_time(&tm, "[%Y-%m-%d %T.%e0] ");
        std::cout << time << "[" << magenta << "script" << reset << "] " << text << endl;
#else
        std::cout << "[script] " << text << endl;
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

    size_t get_class_id(
        const string& module,
        const string& class_name)
    {
        const string concat = module + "::" + class_name;
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

        if (strcmp(module, "meta") == 0)
            return wrenMetaBindForeignMethod(vm, class_name, is_static, signature);

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

        const auto id = get_class_id(module, className);
        const auto itr = foreign_classes.find(id);
        if (itr != foreign_classes.end())
            return itr->second;

        WrenForeignClassMethods res{ NULL, NULL };
        return res;
    }

    WrenLoadModuleResult loadModule(WrenVM* vm, const char* name)
    {
        WrenLoadModuleResult res{};
        if (strcmp(name, "random") == 0)
        {
            res.source = wrenRandomSource();
        }
        else if (strcmp(name, "meta") == 0)
        {
            res.source = wrenMetaSource();
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
    
    log::info("Wren script set to {}", fileio::get_path(internal::main));
    bind_api();

    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn = &writeFn;
    config.errorFn = &errorFn;
    config.bindForeignMethodFn = &bindForeignMethod;
    config.bindForeignClassFn = &bindForeignClass;
    config.loadModuleFn = &loadModule;

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

void xs::script::bind(
    const std::string& module,
    const std::string& class_name,
    WrenForeignMethodFn allocate_fn,
    WrenFinalizerFn finalize_fn)
{
    const auto id = get_class_id(module, class_name);
    foreign_classes[id] = WrenForeignClassMethods{ allocate_fn, finalize_fn };
}

unsigned long xs::script::get_bytes_allocated() {
    return vm->bytesAllocated;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// 
//										All xs API
// 
///////////////////////////////////////////////////////////////////////////////////////////////////

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
    xs::log::error("Expected '{}', received '{}'", ex, rc);

    xs::script::internal::error = true;
    wrenEnsureSlots(vm, 1);
    wrenSetSlotBool(vm, 0, false);
    wrenAbortFiber(xs::script::internal::vm, 0);

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
template<> xs::render::color wrenGetParameter<xs::render::color>(WrenVM* vm, int slot)
{
    xs::render::color c; 
    
    if (checkType(vm, slot, WREN_TYPE_NUM, __func__))
        c.integer_value = wrenGetParameter<uint32_t>(vm, slot);
    else
        c.integer_value = 0;

    return c;
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
void callFunction_args(WrenVM* vm, std::function<void(T)> func)
{
    func(wrenGetParameter<T>(vm, 1));
}

template <typename T1, typename T2>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2));
}

template <typename T1, typename T2, typename T3>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3));
}

template <typename T1, typename T2, typename T3, typename T4>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3, T4)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4));
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3, T4, T5)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4), wrenGetParameter<T5>(vm, 5));
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3, T4, T5, T6)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4), wrenGetParameter<T5>(vm, 5),
        wrenGetParameter<T6>(vm, 6));
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3, T4, T5, T6, T7)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4), wrenGetParameter<T5>(vm, 5),
        wrenGetParameter<T6>(vm, 6), wrenGetParameter<T7>(vm, 7));
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3, T4, T5, T6, T7, T8)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4), wrenGetParameter<T5>(vm, 5),
        wrenGetParameter<T6>(vm, 6), wrenGetParameter<T7>(vm, 7), wrenGetParameter<T8>(vm, 8));
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
void callFunction_args(WrenVM* vm, std::function<void(T1, T2, T3, T4, T5, T6, T7, T8, T9)> func)
{
    func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4), wrenGetParameter<T5>(vm, 5),
        wrenGetParameter<T6>(vm, 6), wrenGetParameter<T7>(vm, 7), wrenGetParameter<T8>(vm, 8), wrenGetParameter<T9>(vm, 9));
}

template <typename ReturnType>
void callFunction_returnType(WrenVM* vm, std::function<ReturnType(void)> func)
{
    wrenSetReturnValue<ReturnType>(vm, func());
}

template <typename ReturnType, typename T>
void callFunction_returnType_args(WrenVM* vm, std::function<ReturnType(T)> func)
{
    wrenSetReturnValue<ReturnType>(vm, func(wrenGetParameter<T>(vm, 1)));
}

template <typename ReturnType, typename T1, typename T2>
void callFunction_returnType_args(WrenVM* vm, std::function<ReturnType(T1, T2)> func)
{
    wrenSetReturnValue<ReturnType>(vm, func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2)));
}

template <typename ReturnType, typename T1, typename T2, typename T3>
void callFunction_returnType_args(WrenVM* vm, std::function<ReturnType(T1, T2, T3)> func)
{
    wrenSetReturnValue<ReturnType>(vm, func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3)));
}

template <typename ReturnType, typename T1, typename T2, typename T3, typename T4>
void callFunction_returnType_args(WrenVM* vm, std::function<ReturnType(T1, T2, T3, T4)> func)
{
    wrenSetReturnValue<ReturnType>(vm, func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4)));
}

template <typename ReturnType, typename T1, typename T2, typename T3, typename T4, typename T5>
void callFunction_returnType_args(WrenVM* vm, std::function<ReturnType(T1, T2, T3, T4, T5)> func)
{
    wrenSetReturnValue<ReturnType>(vm, func(wrenGetParameter<T1>(vm, 1), wrenGetParameter<T2>(vm, 2), wrenGetParameter<T3>(vm, 3), wrenGetParameter<T4>(vm, 4), wrenGetParameter<T5>(vm, 5)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Input
///////////////////////////////////////////////////////////////////////////////////////////////////

void input_get_axis(WrenVM* vm)
{
    callFunction_returnType_args<double, xs::input::gamepad_axis>(vm, xs::input::get_axis);
}

void input_get_button(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::gamepad_button>(vm, xs::input::get_button);
}

void input_get_button_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::gamepad_button>(vm, xs::input::get_button_once);
}

void input_get_key(WrenVM* vm)
{
    callFunction_returnType_args<bool, int>(vm, xs::input::get_key);
}

void input_get_key_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, int>(vm, xs::input::get_key_once);
}

void input_get_mouse(WrenVM* vm)
{
    callFunction_returnType<bool>(vm, xs::input::get_mouse);
}

void input_get_mousebutton(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::mouse_button>(vm, xs::input::get_mousebutton);
}

void input_get_mousebutton_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::mouse_button>(vm, xs::input::get_mousebutton_once);
}

void input_get_mouse_x(WrenVM* vm)
{
    callFunction_returnType<double>(vm, xs::input::get_mouse_x);
}

void input_get_mouse_y(WrenVM* vm)
{
    callFunction_returnType<double>(vm, xs::input::get_mouse_y);
}

void input_get_nr_touches(WrenVM* vm)
{
    callFunction_returnType<int>(vm, xs::input::get_nr_touches);
}

void input_get_touch_id(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::input::get_touch_id);
}

void input_get_touch_x(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::input::get_touch_x);
}

void input_get_touch_y(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::input::get_touch_y);
}

void input_set_gamepad_vibration(WrenVM* vm)
{
    callFunction_args<int,int>(vm, xs::input::set_gamepad_vibration);
}

void input_set_lightbar_color(WrenVM* vm)
{
    // TODO: change argument, use the same color type everywhere?
    callFunction_args<double,double,double>(vm, xs::input::set_lightbar_color);
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
    callFunction_args<xs::render::primitive>(vm, xs::render::begin);
}

void render_end(WrenVM* vm)
{
    xs::render::end();
}

void render_vertex(WrenVM* vm)
{
    callFunction_args<double, double>(vm, xs::render::vertex);
}

void render_set_color(WrenVM* vm)
{
    callFunction_args<xs::render::color>(vm, xs::render::set_color);
}

void render_line(WrenVM* vm)
{
    callFunction_args<double,double,double,double>(vm, xs::render::line);
}

void render_text(WrenVM* vm)
{
    callFunction_args<string,double,double,double>(vm, xs::render::text);
}

void render_load_image(WrenVM* vm)
{
    callFunction_returnType_args<int, string>(vm, xs::render::load_image);
}

void render_get_image_width(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::render::get_image_width);
}

void render_get_image_height(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::render::get_image_height);
}

void render_create_sprite(WrenVM* vm)
{
    callFunction_returnType_args<int,int,double,double,double,double>(vm, xs::render::create_sprite);
}

void render_sprite_ex(WrenVM* vm)
{
    callFunction_args<int,double,double,double,double,double,xs::render::color, xs::render::color, uint32_t>(vm, xs::render::render_sprite);
}

void render_set_offset(WrenVM* vm)
{
    callFunction_args<double, double>(vm, xs::render::set_offset);
}

void render_load_font(WrenVM* vm)
{
    callFunction_returnType_args<int, string, double>(vm, xs::render::load_font);
}

void render_render_text(WrenVM* vm)
{
    callFunction_args<int, string, double, double, xs::render::color, xs::render::color, uint32_t>(vm, xs::render::render_text);
}

void render_create_shape(WrenVM* vm)
{
	auto pcount = wrenGetListCount(vm, 1);
	auto ccount = wrenGetListCount(vm, 2);
	
	if(pcount != ccount * 2) {
		// TODO: Print error
	}
	
	vector<double> points(pcount);
	for(int i = 0; i < pcount; i++)
	{
		wrenGetListElement(vm, 1, i, 0);		// Check data first
		points[i] = wrenGetSlotDouble(vm, 0);
	}
	
	vector<xs::render::color> colors(ccount);
	for(int i = 0; i < ccount; i++)
	{
		wrenGetListElement(vm, 2, i, 0);
		xs::render::color c;
		auto dc = wrenGetSlotDouble(vm, 0);
		c.integer_value = static_cast<unsigned int>(dc);
		colors[i] = c;
	}
	
	auto id = xs::render::create_shape(points, colors);
	wrenSetSlotDouble(vm, 0, id);
}

void render_render_shape(WrenVM* vm)
{
	callFunction_args<int, double, double, double, double, xs::render::color, xs::render::color>(vm, xs::render::render_shape);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Audio
///////////////////////////////////////////////////////////////////////////////////////////////////

void audio_load(WrenVM* vm)
{
    callFunction_returnType_args<int, string, int>(vm, xs::audio::load);
}

void audio_play(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::audio::play);
}

void audio_get_group_volume(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::audio::get_group_volume);
}

void audio_set_group_volume(WrenVM* vm)
{
    callFunction_args<int, double>(vm, xs::audio::set_group_volume);
}

void audio_get_channel_volume(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::audio::get_channel_volume);
}

void audio_set_channel_volume(WrenVM* vm)
{
    callFunction_args<int, double>(vm, xs::audio::set_channel_volume);
}

void audio_get_bus_volume(WrenVM* vm)
{
    callFunction_returnType_args<double, string>(vm, xs::audio::get_bus_volume);
}

void audio_set_bus_volume(WrenVM* vm)
{
    callFunction_args<string, double>(vm, xs::audio::set_bus_volume);
}

void audio_load_bank(WrenVM* vm)
{
    callFunction_returnType_args<int, string>(vm, xs::audio::load_bank);
}

void audio_unload_bank(WrenVM* vm)
{
    callFunction_args<int>(vm, xs::audio::unload_bank);
}

void audio_start_event(WrenVM* vm)
{
    callFunction_returnType_args<int, string>(vm, xs::audio::start_event);
}

void audio_set_parameter_number(WrenVM* vm)
{
    callFunction_args<int, string, double>(vm, xs::audio::set_parameter_number);
}

void audio_set_parameter_label(WrenVM* vm)
{
    callFunction_args<int, string, string>(vm, xs::audio::set_parameter_label);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////////////////////////

void data_get_number(WrenVM* vm)
{
    callFunction_returnType_args<double, string, xs::data::type>(vm, xs::data::get_number);
}

void data_get_bool(WrenVM* vm)
{
    callFunction_returnType_args<bool, string, xs::data::type>(vm, xs::data::get_bool);
}

void data_get_color(WrenVM* vm)
{
    callFunction_returnType_args<uint32_t, string, xs::data::type>(vm, xs::data::get_color);
}

void data_get_string(WrenVM* vm)
{
    callFunction_returnType_args<string, string, xs::data::type>(vm, xs::data::get_string);
}

void data_set_bool(WrenVM* vm)
{
    callFunction_args<string, bool, xs::data::type>(vm, xs::data::set_bool);
}

void data_set_number(WrenVM* vm)
{
    callFunction_args<string, double, xs::data::type>(vm, xs::data::set_number);
}

void data_set_color(WrenVM* vm)
{
    callFunction_args<string, uint32_t, xs::data::type>(vm, xs::data::set_color);
}

void data_set_string(WrenVM* vm)
{
    callFunction_args<string, string, xs::data::type>(vm, xs::data::set_string);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// File
///////////////////////////////////////////////////////////////////////////////////////////////////

void file_read(WrenVM* vm)
{
    callFunction_returnType_args<string, string>(vm, xs::fileio::read_text_file);
}

void file_write(WrenVM* vm)
{
    callFunction_returnType_args<bool, string, string>(vm, xs::fileio::write_text_file);
}

void file_exists(WrenVM* vm)
{
    callFunction_returnType_args<bool, string>(vm, xs::fileio::exists);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Device
///////////////////////////////////////////////////////////////////////////////////////////////////

void device_get_platform(WrenVM* vm)
{
    callFunction_returnType<xs::device::platform>(vm, xs::device::get_platform);
}

void device_can_close(WrenVM* vm)
{
    callFunction_returnType<bool>(vm, xs::device::can_close);
}

void device_request_close(WrenVM* vm)
{
    callFunction_returnType<bool>(vm, xs::device::request_close);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Profiler
///////////////////////////////////////////////////////////////////////////////////////////////////
void profiler_begin_section(WrenVM* vm)
{
	callFunction_args<string>(vm, xs::profiler::begin_section);
}

void profiler_end_section(WrenVM* vm)
{
	callFunction_args<string>(vm, xs::profiler::end_section);
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
    bind("xs", "Render", true, "setColor(_)", render_set_color);
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
	bind("xs", "Render", true, "createShape(_,_)", render_create_shape);
	bind("xs", "Render", true, "shape(_,_,_,_,_,_,_)", render_render_shape);

    // Audio
    bind("xs", "Audio", true, "load(_,_)", audio_load);
    bind("xs", "Audio", true, "play(_)", audio_play);
    bind("xs", "Audio", true, "getGroupVolume(_)", audio_get_group_volume);
    bind("xs", "Audio", true, "setGroupVolume(_,_)", audio_set_group_volume);
    bind("xs", "Audio", true, "getChannelVolume(_)", audio_get_channel_volume);
    bind("xs", "Audio", true, "setChannelVolume(_,_)", audio_set_channel_volume);
    bind("xs", "Audio", true, "getBusVolume(_)", audio_get_bus_volume);
    bind("xs", "Audio", true, "setBusVolume(_,_)", audio_set_bus_volume);
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

    // Profiler
    bind("xs", "Profiler", true, "begin(_)", profiler_begin_section);
    bind("xs", "Profiler", true, "end(_)", profiler_end_section);
}
