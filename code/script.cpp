#include <cassert>
#include <iomanip>
#include <iostream>
#include <functional>
#include <type_traits>
#include <string>
#include <unordered_map>
#include <array>
#include <wren.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "fileio.hpp"
#include "log.hpp"
#include "profiler.hpp"
#include "data.hpp"
#include "input.hpp"
#include "tools.hpp"
#include "render.hpp"
#include "script.hpp"
#include "configuration.hpp"
#include "fileio.hpp"
#include "audio.hpp"
#include "simple_audio.hpp"
#include "device.hpp"
#include "inspector.hpp"
#include "color.hpp"
#include "json/json.hpp"
#include <imgui.h>

// Check if we are running MSVC
#ifdef _MSC_VER
// Disable warning about zero-sized arrays in structs
#pragma warning(disable:4200)
#endif

#include <wren.hpp>
extern "C" {
#include "wren/optional/wren_opt_random.h"
#include "wren/optional/wren_opt_meta.h"
#include "wren_vm.h"
}

// Re-enable the warning if we are running MSVC
#ifdef _MSC_VER
#pragma warning(default:4200)
#endif

using namespace std;
using namespace xs;


namespace xs::script
{
    string main;
}


namespace xs::script::internal
{
    WrenVM* vm = nullptr;
    WrenHandle* game_class = nullptr;
    WrenHandle* init_method = nullptr;
    WrenHandle* update_method = nullptr;
    WrenHandle* render_method = nullptr;
    std::unordered_map<size_t, WrenForeignMethodFn> foreign_methods;
    std::unordered_map<size_t, WrenForeignClassMethods> foreign_classes;
    struct module { string path; string source; };
    std::unordered_map<string, module> modules; // name to source mapping
    bool initialized = false;
    bool error = false;

    void writeFn(WrenVM* vm, const char* text)
    {
        if (strcmp(text, "\n") == 0)
            return;

        xs::log::script("{}", text);
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
            case WREN_ERROR_STACK_TRACE:
            {
                string smodule(module);
                auto path = modules[smodule].path;
                auto abs = xs::fileio::absolute(path);            
                xs::log::error("[{}:{}] in {}", abs, line, msg);
            } break;
            case WREN_ERROR_RUNTIME:
            {
                xs::log::error("[Runtime Error] {}", msg);
				xs::inspector::notify(xs::inspector::notification_type::error,
									  "Runtime Error: " + string(msg), 5.0f);
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

    size_t get_class_id(const string& module, const string& class_name)
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
            string sname(name);
            auto filename = "[shared]/modules/" + sname + ".wren";
            if (!xs::fileio::exists(filename))
            {
                auto mstring = string(main);
                auto i = mstring.find_last_of('/');
                filename = mstring.erase(i) + '/' + sname + ".wren";
                if (!xs::fileio::exists(filename))
                {
                    log::warn("Module '{}' can not be found!", name);
                }
            }
            auto& m = modules[sname];
            m.path = filename;
            m.source = xs::fileio::read_text_file(filename);
            res.source = m.source.c_str();
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

    main = fileio::get_path("[game]/[main]"); 

    if (!fileio::exists(main))
    {
        log::error("Wren script file {} not found!", main);
        log::error("Please restart with a valid script file!");
        return;
    }

    bind_api();

    WrenConfiguration config;
    wrenInitConfiguration(&config);
    config.writeFn = &writeFn;
    config.errorFn = &errorFn;
    config.bindForeignMethodFn = &bindForeignMethod;
    config.bindForeignClassFn = &bindForeignClass;
    config.loadModuleFn = &loadModule;    
    vm = wrenNewVM(&config);

    const string& script_file = fileio::read_text_file(main);
    modules["game"].path = main;
    modules["game"].source = script_file;
    const WrenInterpretResult result = wrenInterpret(vm, "game", script_file.c_str());

    switch (result)
    {
    case WREN_RESULT_COMPILE_ERROR:
    {
        log::error("Compile Error!");
        inspector::notify(inspector::notification_type::error, "Compile Error!", 10.0f);
        error = true;
    } break;
    case WREN_RESULT_RUNTIME_ERROR:
    {
        log::error("Runtime Error!");
        inspector::notify(inspector::notification_type::error, "Runtime Error!", 10.0f);
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
        auto message = string("Game compile success. ") + pr;
        log::info(message);
        inspector::notify(inspector::notification_type::success, message, 5.0f);
    } break;
    }

    if (initialized && !error)
    {
        wrenEnsureSlots(vm, 1);									// Make sure there at least one slot
        wrenGetVariable(vm, "game", "Game", 0);		            // Grab a handle to the Game class
        game_class = wrenGetSlotHandle(vm, 0);
        wrenSetSlotHandle(vm, 0, game_class);					// Put Game class in slot 0
        init_method = wrenMakeCallHandle(vm, "initialize()");
        update_method = wrenMakeCallHandle(vm, "update(_)");
        render_method = wrenMakeCallHandle(vm, "render()");
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

void xs::script::ec_inspect(const std::string& filter)
{
    if (!initialized)
        return;

    if (modules.find("xs/ec") == modules.end())
    {
        ImGui::Text("The Entity Component (xs/ec) module is not loaded");
        return;
    }

    // Try to get the Entity class from xs_ec module
    wrenEnsureSlots(vm, 2);
    wrenGetVariable(vm, "xs/ec", "Entity", 0);

    // Check if the class was found (not null)
    if (wrenGetSlotType(vm, 0) == WREN_TYPE_NULL)
    {
        ImGui::Text("The Entity Component (xs/ec) module is not loaded");
        return;
    }

    // Call Entity.inspect(filter)
    WrenHandle* entity_class = wrenGetSlotHandle(vm, 0);
    WrenHandle* inspect_method = wrenMakeCallHandle(vm, "inspect(_)");

    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, entity_class);
    wrenSetSlotString(vm, 1, filter.c_str());
    wrenCall(vm, inspect_method);

    // Clean up handles
    wrenReleaseHandle(vm, inspect_method);
    wrenReleaseHandle(vm, entity_class);
}

bool script::is_module_loaded(const std::string& module)
{
    return modules.find(module) != modules.end();
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

void bind_class(
    const string& module,
    const string& class_name,
    WrenForeignClassMethods methods)
{
	const auto id = get_class_id(module, class_name);
	foreign_classes[id] = methods;
}

size_t xs::script::get_bytes_allocated()
{
	return vm ? vm->bytesAllocated : 0;
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
        return "foreign";
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

template <typename T> T wrenGetParameter(WrenVM* vm, int slot);

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
template<> xs::color wrenGetParameter<xs::color>(WrenVM* vm, int slot)
{
    xs::color c;     
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

template<> glm::vec4 wrenGetParameter<glm::vec4>(WrenVM* vm, int slot)
{
    if(checkType(vm, slot, WREN_TYPE_FOREIGN, __func__))
		return *static_cast<glm::vec4*>(wrenGetSlotForeign(vm, slot));
    return glm::vec4(0.0f);
}

template<> tools::handle wrenGetParameter<tools::handle>(WrenVM* vm, int slot)
{
	if (checkType(vm, slot, WREN_TYPE_FOREIGN, __func__))
		return *static_cast<tools::handle*>(wrenGetSlotForeign(vm, slot));
	return tools::handle();
}

template <typename T>
T wrenGetParameter(WrenVM* vm, int slot)
{
    if (checkType(vm, slot, WREN_TYPE_NUM, __func__))
        return (T)wrenGetSlotDouble(vm, slot);
    return (T)0;
}

template <typename T> std::vector<T> wrenGetListParameter(WrenVM* vm, int slot)
{
    std::vector<T> values;
    if (checkType(vm, slot, WREN_TYPE_LIST, __func__))
    {
        auto count = wrenGetListCount(vm, slot);
        values.resize(count);
        for (int i = 0; i < count; i++)
        {
            wrenGetListElement(vm, slot, i, 0);
            values[i] = wrenGetParameter<T>(vm, 0);
        }
    }
    return values;
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

static void input_get_axis(WrenVM* vm)
{
    callFunction_returnType_args<double, xs::input::gamepad_axis>(vm, xs::input::get_axis);
}

static void input_get_axis_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::gamepad_axis, double>(vm, xs::input::get_axis_once);
}

static void input_get_button(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::gamepad_button>(vm, xs::input::get_button);
}

static void input_get_button_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::gamepad_button>(vm, xs::input::get_button_once);
}

static void input_get_key(WrenVM* vm)
{
    callFunction_returnType_args<bool, int>(vm, xs::input::get_key);
}

static void input_get_key_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, int>(vm, xs::input::get_key_once);
}

static void input_get_mouse(WrenVM* vm)
{
    callFunction_returnType<bool>(vm, xs::input::get_mouse);
}

static void input_get_mousebutton(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::mouse_button>(vm, xs::input::get_mousebutton);
}

static void input_get_mousebutton_once(WrenVM* vm)
{
    callFunction_returnType_args<bool, xs::input::mouse_button>(vm, xs::input::get_mousebutton_once);
}

static void input_get_mouse_x(WrenVM* vm)
{
#ifdef INSPECTOR
    wrenSetSlotDouble(vm, 0, inspector::get_game_mouse_x());
#else
    callFunction_returnType<double>(vm, xs::input::get_mouse_x);
#endif
}

static void input_get_mouse_y(WrenVM* vm)
{
#ifdef INSPECTOR
    wrenSetSlotDouble(vm, 0, inspector::get_game_mouse_y());
#else
    callFunction_returnType<double>(vm, xs::input::get_mouse_y);
#endif
}

static void input_get_mouse_wheel(WrenVM* vm)
{
    callFunction_returnType<double>(vm, xs::input::get_mouse_wheel);
}

static void input_get_nr_touches(WrenVM* vm)
{
    callFunction_returnType<int>(vm, xs::input::get_nr_touches);
}

static void input_get_touch_id(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::input::get_touch_id);
}

static void input_get_touch_x(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::input::get_touch_x);
}

static void input_get_touch_y(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::input::get_touch_y);
}

static void input_set_gamepad_vibration(WrenVM* vm)
{
    callFunction_args<double, double, double>(vm, xs::input::set_gamepad_vibration);
}

static void input_set_lightbar_color(WrenVM* vm)
{
    // TODO: change argument, use the same color type everywhere?
    callFunction_args<double,double,double>(vm, xs::input::set_lightbar_color);
}

static void input_reset_lightbar(WrenVM* vm)
{
    xs::input::reset_lightbar();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////////////////////////

static void render_dbg_begin(WrenVM* vm)
{
    callFunction_args<xs::render::dbg_primitive>(vm, xs::render::dgb_begin);
}

static void render_dbg_end(WrenVM* vm)
{
    xs::render::dbg_end();
}

static void render_dbg_vertex(WrenVM* vm)
{
    callFunction_args<double, double>(vm, xs::render::dbg_vertex);
}

static void render_dbg_color(WrenVM* vm)
{
    // Call manually
    auto c = wrenGetParameter<xs::color>(vm, 1);
    xs::render::dbg_color(c);

    // callFunction_args<xs::render::color>(vm, xs::render::set_color);
}

static void render_dbg_line(WrenVM* vm)
{
    callFunction_args<double,double,double,double>(vm, xs::render::dbg_line);
}

static void render_dbg_text(WrenVM* vm)
{
    callFunction_args<string,double,double,double>(vm, xs::render::dbg_text);
}

static void render_load_image(WrenVM* vm)
{
    callFunction_returnType_args<int, string>(vm, xs::render::load_image);
}

static void render_load_shape(WrenVM* vm)
{
    auto shape_path = wrenGetParameter<string>(vm, 1);
    auto shape_id = xs::render::load_shape(shape_path);
    wrenGetVariable(vm, "xs/core", "ShapeHandle", 0);
    auto handle = (int*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(int));
    *handle = shape_id;
    
    // callFunction_returnType_args<int, string>(vm, xs::render::load_shape);
}

static void render_get_image_width(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::render::get_image_width);
}

static void render_get_image_height(WrenVM* vm)
{
    callFunction_returnType_args<int, int>(vm, xs::render::get_image_height);
}

static void render_create_sprite(WrenVM* vm)
{
    auto image_id = wrenGetParameter<int>(vm, 1);
    auto x0 = wrenGetParameter<double>(vm, 2);
    auto y0 = wrenGetParameter<double>(vm, 3);
    auto x1 = wrenGetParameter<double>(vm, 4);
    auto y1 = wrenGetParameter<double>(vm, 5);
    auto sprite_id = xs::render::create_sprite(image_id, x0, y0, x1, y1);

    wrenGetVariable(vm, "xs/core", "ShapeHandle", 0);
    auto handle = (int*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(int));
    *handle = sprite_id;
    
}

static void render_create_shape(WrenVM* vm)
{
    auto image_id = wrenGetParameter<int>(vm, 1);
	auto positions = wrenGetListParameter<float>(vm, 2);
	auto texture_coordinates = wrenGetListParameter<float>(vm, 3);
	auto indices = wrenGetListParameter<unsigned short>(vm, 4);

    auto shape_id = xs::render::create_shape(
		image_id,
		positions.data(),
		texture_coordinates.data(),
		(unsigned int)positions.size() / 2,
		indices.data(),
		(unsigned int)indices.size());

    wrenGetVariable(vm, "xs/core", "ShapeHandle", 0);
	auto handle = (int*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(int));
	*handle = shape_id;
}

static void render_destroy_shape(WrenVM* vm)
{
    auto handle = (int*)wrenGetSlotForeign(vm, 1);
	xs::render::destroy_shape(*handle);
}

static void render_sprite(WrenVM* vm)
{
    callFunction_args<
        tools::handle,
        double,
        double,
        double,
        double,
        double,
        xs::color,
        xs::color,
        uint32_t
    >(vm, xs::render::sprite);    
}

static void render_shape(WrenVM* vm)
{
	callFunction_args<
		tools::handle,
		double,
		double,
		double,
		double,
		double,
		xs::color,
		xs::color
	>(vm, xs::render::shape);
}

static void render_set_offset(WrenVM* vm)
{
    callFunction_args<double, double>(vm, xs::render::set_offset);
}

static void render_load_font(WrenVM* vm)
{
    callFunction_returnType_args<int, string, double>(vm, xs::render::load_font);
}

void render_text(WrenVM* vm)
{
    callFunction_args<int, string, double, double, double, xs::color, xs::color, uint32_t>(vm, xs::render::text);
}

/*
void render_render_shape(WrenVM* vm)
{
	callFunction_args<int, double, double, double, double, xs::render::color, xs::render::color>(vm, xs::render::render_sprite);
}
*/

void shape_handle_allocate(WrenVM* vm)
{
	void* data = wrenSetSlotNewForeign(vm, 0, 0, sizeof(int));
	auto handle = new (data) int();
	*handle = -1;
}

void shape_handle_finalize(void* data)
{
	auto handle = static_cast<int*>(data);
    if (*handle != -1)
    {
		xs::render::destroy_shape(*handle);
		*handle = -1;
	}
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
    callFunction_returnType_args<int, int>(vm, xs::audio::play);
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
// SimpleAudio
///////////////////////////////////////////////////////////////////////////////////////////////////

void simple_audio_load(WrenVM* vm)
{
    callFunction_returnType_args<int, string>(vm, xs::simple_audio::load);
}

void simple_audio_play(WrenVM* vm)
{
    callFunction_returnType_args<double, int, double>(vm, xs::simple_audio::play);
}

void simple_audio_set_volume(WrenVM* vm)
{
    callFunction_args<int, double>(vm, xs::simple_audio::set_volume);
}

void simple_audio_get_volume(WrenVM* vm)
{
    callFunction_returnType_args<double, int>(vm, xs::simple_audio::get_volume);
}

void simple_audio_stop(WrenVM* vm)
{
    callFunction_args<int>(vm, xs::simple_audio::stop);
}

void simple_audio_stop_all(WrenVM* vm)
{
    xs::simple_audio::stop_all();
}

void simple_audio_is_playing(WrenVM* vm)
{
    callFunction_returnType_args<bool, int>(vm, xs::simple_audio::is_playing);
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
// JSON
///////////////////////////////////////////////////////////////////////////////////////////////////

// Recursively converts nlohmann::json to Wren value
void json_to_wren(WrenVM* vm, int slot, const nlohmann::json& j)
{
    if (j.is_null())
    {
        wrenSetSlotNull(vm, slot);
    }
    else if (j.is_boolean())
    {
        wrenSetSlotBool(vm, slot, j.get<bool>());
    }
    else if (j.is_number())
    {
        wrenSetSlotDouble(vm, slot, j.get<double>());
    }
    else if (j.is_string())
    {
        wrenSetSlotString(vm, slot, j.get<string>().c_str());
    }
    else if (j.is_array())
    {
        wrenSetSlotNewList(vm, slot);
        int elemSlot = slot + 1;
        wrenEnsureSlots(vm, elemSlot + 1);
        for (size_t i = 0; i < j.size(); i++)
        {
            json_to_wren(vm, elemSlot, j[i]);
            wrenInsertInList(vm, slot, -1, elemSlot);
        }
    }
    else if (j.is_object())
    {
        wrenSetSlotNewMap(vm, slot);
        int keySlot = slot + 1;
        int valueSlot = slot + 2;
        wrenEnsureSlots(vm, valueSlot + 1);
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            wrenSetSlotString(vm, keySlot, it.key().c_str());
            json_to_wren(vm, valueSlot, it.value());
            wrenSetMapValue(vm, slot, keySlot, valueSlot);
        }
    }
}

// Recursively converts Wren value to nlohmann::json
nlohmann::json wren_to_json(WrenVM* vm, int slot)
{
    WrenType type = wrenGetSlotType(vm, slot);

    switch (type)
    {
    case WREN_TYPE_NULL:
        return nlohmann::json(nullptr);
    case WREN_TYPE_BOOL:
        return nlohmann::json(wrenGetSlotBool(vm, slot));
    case WREN_TYPE_NUM:
        return nlohmann::json(wrenGetSlotDouble(vm, slot));
    case WREN_TYPE_STRING:
        return nlohmann::json(string(wrenGetSlotString(vm, slot)));
    case WREN_TYPE_LIST:
    {
        nlohmann::json arr = nlohmann::json::array();
        int count = wrenGetListCount(vm, slot);
        int elemSlot = slot + 1;
        wrenEnsureSlots(vm, elemSlot + 1);
        for (int i = 0; i < count; i++)
        {
            wrenGetListElement(vm, slot, i, elemSlot);
            arr.push_back(wren_to_json(vm, elemSlot));
        }
        return arr;
    }
    case WREN_TYPE_MAP:
    {
        nlohmann::json obj = nlohmann::json::object();
        int count = wrenGetMapCount(vm, slot);
        // Get map keys as a list
        int keysSlot = slot + 1;
        int keySlot = slot + 2;
        int valueSlot = slot + 3;
        wrenEnsureSlots(vm, valueSlot + 1);

        // Iterate through the map - we need to get keys first
        // Unfortunately Wren doesn't have a direct way to iterate maps,
        // so we'll need to use a workaround by calling the keys method
        // For now, return empty object for maps (limitation)
        xs::log::warn("JSON stringify: Map conversion not fully supported");
        return obj;
    }
    default:
        return nlohmann::json(nullptr);
    }
}

void json_load(WrenVM* vm)
{
    auto path = wrenGetParameter<string>(vm, 1);

    if (!xs::fileio::exists(path))
    {
        xs::log::error("JSON load: File not found: {}", path);
        wrenSetSlotNull(vm, 0);
        return;
    }

    auto content = xs::fileio::read_text_file(path);
    if (content.empty())
    {
        wrenSetSlotNull(vm, 0);
        return;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(content);
        json_to_wren(vm, 0, j);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        xs::log::error("JSON parse error in '{}': {}", path, e.what());
        wrenSetSlotNull(vm, 0);
    }
}

void json_parse(WrenVM* vm)
{
    auto content = wrenGetParameter<string>(vm, 1);

    try
    {
        nlohmann::json j = nlohmann::json::parse(content);
        json_to_wren(vm, 0, j);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        xs::log::error("JSON parse error: {}", e.what());
        wrenSetSlotNull(vm, 0);
    }
}

void json_save(WrenVM* vm)
{
    auto path = wrenGetParameter<string>(vm, 1);
    // Slot 2 contains the value to save

    try
    {
        nlohmann::json j = wren_to_json(vm, 2);
        auto content = j.dump(4); // Pretty print with 4 spaces
        bool success = xs::fileio::write_text_file(content, path);
        wrenSetSlotBool(vm, 0, success);
    }
    catch (const std::exception& e)
    {
        xs::log::error("JSON save error: {}", e.what());
        wrenSetSlotBool(vm, 0, false);
    }
}

void json_stringify(WrenVM* vm)
{
    // Slot 1 contains the value to stringify
    try
    {
        nlohmann::json j = wren_to_json(vm, 1);
        auto content = j.dump(4); // Pretty print with 4 spaces
        wrenSetSlotString(vm, 0, content.c_str());
    }
    catch (const std::exception& e)
    {
        xs::log::error("JSON stringify error: {}", e.what());
        wrenSetSlotString(vm, 0, "null");
    }
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

void device_set_fullscreen(WrenVM* vm)
{
    callFunction_args<bool>(vm, xs::device::set_fullscreen);
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
// Inspector (ImGui bindings) - Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
void inspector_checkbox(WrenVM* vm);
void inspector_begin_child(WrenVM* vm);
void inspector_end_child(WrenVM* vm);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Inspector (ImGui bindings)
///////////////////////////////////////////////////////////////////////////////////////////////////
void inspector_text(WrenVM* vm)
{
	auto text = wrenGetParameter<string>(vm, 1);
	ImGui::Text("%s", text.c_str());
}

void inspector_tree_node(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	bool result = ImGui::TreeNode(label.c_str());
	wrenSetSlotBool(vm, 0, result);
}

void inspector_tree_pop(WrenVM* vm)
{
	ImGui::TreePop();
}

void inspector_separator(WrenVM* vm)
{
	ImGui::Separator();
}

void inspector_separator_text(WrenVM* vm)
{
	auto text = wrenGetParameter<string>(vm, 1);
	ImGui::SeparatorText(text.c_str());
}

void inspector_same_line(WrenVM* vm)
{
	ImGui::SameLine();
}

void inspector_indent(WrenVM* vm)
{
	ImGui::Indent();
}

void inspector_unindent(WrenVM* vm)
{
	ImGui::Unindent();
}

void inspector_spacing(WrenVM* vm)
{
	ImGui::Spacing();
}

void inspector_selectable(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	auto selected = wrenGetParameter<bool>(vm, 2);
	bool result = ImGui::Selectable(label.c_str(), selected);
	wrenSetSlotBool(vm, 0, result);
}

void inspector_input_float(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	auto value = wrenGetParameter<double>(vm, 2);
	float f = (float)value;
	bool changed = ImGui::InputFloat(label.c_str(), &f);
	if (changed) {
		wrenSetSlotDouble(vm, 0, (double)f);
	} else {
		wrenSetSlotDouble(vm, 0, value);
	}
}

void inspector_drag_float(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	auto value = wrenGetParameter<double>(vm, 2);
	float f = (float)value;
	bool changed = ImGui::DragFloat(label.c_str(), &f, 0.1f);
	if (changed) {
		wrenSetSlotDouble(vm, 0, (double)f);
	} else {
		wrenSetSlotDouble(vm, 0, value);
	}
}

void inspector_collapsing_header(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	bool result = ImGui::CollapsingHeader(label.c_str());
	wrenSetSlotBool(vm, 0, result);
}

void inspector_drag_float2(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	auto x = wrenGetParameter<double>(vm, 2);
	auto y = wrenGetParameter<double>(vm, 3);
	
	float values[2] = { (float)x, (float)y };
	bool changed = ImGui::DragFloat2(label.c_str(), values, 0.1f);
	
	// Return as a list [x, y]
	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);
	wrenSetSlotDouble(vm, 1, (double)values[0]);
	wrenInsertInList(vm, 0, 0, 1);
	wrenSetSlotDouble(vm, 2, (double)values[1]);
	wrenInsertInList(vm, 0, 1, 2);
}

void inspector_checkbox(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	auto value = wrenGetParameter<bool>(vm, 2);
	bool changed = ImGui::Checkbox(label.c_str(), &value);
	wrenSetSlotBool(vm, 0, value);
}

void inspector_begin_child(WrenVM* vm)
{
	auto label = wrenGetParameter<string>(vm, 1);
	auto width = wrenGetParameter<double>(vm, 2);
	auto height = wrenGetParameter<double>(vm, 3);
	auto border = wrenGetParameter<bool>(vm, 4);

	// If size is between 0 and 1 (exclusive), treat as fraction of available space
	ImVec2 available = ImGui::GetContentRegionAvail();
	float w = (width > 0.0 && width < 1.0) ? (float)(width * available.x) : (float)width;
	float h = (height > 0.0 && height < 1.0) ? (float)(height * available.y) : (float)height;

	ImGui::BeginChild(label.c_str(), ImVec2(w, h), border);
}

void inspector_end_child(WrenVM* vm)
{
	ImGui::EndChild();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//											Bind xs API
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void xs::script::bind_api()
{
    // Input
    bind("xs/core", "Input", true, "getAxis(_)", input_get_axis);
    bind("xs/core", "Input", true, "getAxisOnce(_,_)", input_get_axis_once);
    bind("xs/core", "Input", true, "getButton(_)", input_get_button);
    bind("xs/core", "Input", true, "getButtonOnce(_)", input_get_button_once);
    bind("xs/core", "Input", true, "getKey(_)", input_get_key);
    bind("xs/core", "Input", true, "getKeyOnce(_)", input_get_key_once);
    bind("xs/core", "Input", true, "getMouse()", input_get_mouse);
    bind("xs/core", "Input", true, "getMouseButton(_)", input_get_mousebutton);
    bind("xs/core", "Input", true, "getMouseButtonOnce(_)", input_get_mousebutton_once);
    bind("xs/core", "Input", true, "getMouseX()", input_get_mouse_x);
    bind("xs/core", "Input", true, "getMouseY()", input_get_mouse_y);
    bind("xs/core", "Input", true, "getMouseWheel()", input_get_mouse_wheel);
    bind("xs/core", "Input", true, "getNrTouches()", input_get_nr_touches);
    bind("xs/core", "Input", true, "getTouchId(_)", input_get_touch_id);
    bind("xs/core", "Input", true, "getTouchX(_)", input_get_touch_x);
    bind("xs/core", "Input", true, "getTouchY(_)", input_get_touch_y);
    bind("xs/core", "Input", true, "setPadVibration(_,_,_)", input_set_gamepad_vibration);
    bind("xs/core", "Input", true, "setPadLightbarColor(_,_,_)", input_set_lightbar_color);
    bind("xs/core", "Input", true, "resetPadLightbarColor()", input_reset_lightbar);

    // Render
    bind("xs/core", "Render", true, "loadImage(_)", render_load_image);
    bind("xs/core", "Render", true, "loadShape(_)", render_load_shape);
    bind("xs/core", "Render", true, "getImageWidth(_)", render_get_image_width);
    bind("xs/core", "Render", true, "getImageHeight(_)", render_get_image_height);
    bind("xs/core", "Render", true, "createSprite(_,_,_,_,_)", render_create_sprite);
    bind("xs/core", "Render", true, "createShape(_,_,_,_)", render_create_shape);
    bind("xs/core", "Render", true, "setOffset(_,_)", render_set_offset);
    bind("xs/core", "Render", true, "sprite(_,_,_,_,_,_,_,_,_)", render_sprite);
    bind("xs/core", "Render", true, "shape(_,_,_,_,_,_,_,_)", render_shape);
    bind("xs/core", "Render", true, "loadFont(_,_)", render_load_font);
    bind("xs/core", "Render", true, "text(_,_,_,_,_,_,_,_)", render_text);
    bind("xs/core", "Render", true, "destroyShape(_)", render_destroy_shape);
    bind("xs/core", "Render", true, "dbgBegin(_)", render_dbg_begin);
    bind("xs/core", "Render", true, "dbgEnd()", render_dbg_end);
    bind("xs/core", "Render", true, "dbgVertex(_,_)", render_dbg_vertex);
    bind("xs/core", "Render", true, "dbgColor(_)", render_dbg_color);
    bind("xs/core", "Render", true, "dbgText(_,_,_,_)", render_dbg_text);
    bind("xs/core", "Render", true, "dbgLine(_,_,_,_)", render_dbg_line);


    // ShapeHandle
    WrenForeignClassMethods shape_handle_methods{};
    shape_handle_methods.allocate = shape_handle_allocate;
    shape_handle_methods.finalize = shape_handle_finalize;
    bind_class("xs/core", "ShapeHandle", shape_handle_methods);

    // Audio
    bind("xs/core", "Audio", true, "load(_,_)", audio_load);
    bind("xs/core", "Audio", true, "play(_)", audio_play);
    bind("xs/core", "Audio", true, "getGroupVolume(_)", audio_get_group_volume);
    bind("xs/core", "Audio", true, "setGroupVolume(_,_)", audio_set_group_volume);
    bind("xs/core", "Audio", true, "getChannelVolume(_)", audio_get_channel_volume);
    bind("xs/core", "Audio", true, "setChannelVolume(_,_)", audio_set_channel_volume);
    bind("xs/core", "Audio", true, "getBusVolume(_)", audio_get_bus_volume);
    bind("xs/core", "Audio", true, "setBusVolume(_,_)", audio_set_bus_volume);
    bind("xs/core", "Audio", true, "loadBank(_)", audio_load_bank);
    bind("xs/core", "Audio", true, "unloadBank(_)", audio_unload_bank);
    bind("xs/core", "Audio", true, "startEvent(_)", audio_start_event);
    bind("xs/core", "Audio", true, "setParameterNumber(_,_,_)", audio_set_parameter_number);
    bind("xs/core", "Audio", true, "setParameterLabel(_,_,_)", audio_set_parameter_label);

    // SimpleAudio
    bind("xs/core", "SimpleAudio", true, "load(_)", simple_audio_load);
    bind("xs/core", "SimpleAudio", true, "play(_,_)", simple_audio_play);
    bind("xs/core", "SimpleAudio", true, "setVolume(_,_)", simple_audio_set_volume);
    bind("xs/core", "SimpleAudio", true, "getVolume(_)", simple_audio_get_volume);
    bind("xs/core", "SimpleAudio", true, "stop(_)", simple_audio_stop);
    bind("xs/core", "SimpleAudio", true, "stopAll()", simple_audio_stop_all);
    bind("xs/core", "SimpleAudio", true, "isPlaying(_)", simple_audio_is_playing);

    // Data
    bind("xs/core", "Data", true, "getNumber(_,_)", data_get_number);
    bind("xs/core", "Data", true, "getColor(_,_)", data_get_color);
    bind("xs/core", "Data", true, "getBool(_,_)", data_get_bool);
    bind("xs/core", "Data", true, "getString(_,_)", data_get_string);
    bind("xs/core", "Data", true, "setNumber(_,_,_)", data_set_number);
    bind("xs/core", "Data", true, "setColor(_,_,_)", data_set_color);
    bind("xs/core", "Data", true, "setBool(_,_,_)", data_set_bool);
    bind("xs/core", "Data", true, "setString(_,_,_)", data_set_string);

    // File
    bind("xs/core", "File", true, "read(_)", file_read);
    bind("xs/core", "File", true, "write(_,_)", file_write);
    bind("xs/core", "File", true, "exists(_)", file_exists);

    // JSON
    bind("xs/core", "Json", true, "load(_)", json_load);
    bind("xs/core", "Json", true, "parse(_)", json_parse);
    bind("xs/core", "Json", true, "save(_,_)", json_save);
    bind("xs/core", "Json", true, "stringify(_)", json_stringify);

    // Device
    bind("xs/core", "Device", true, "getPlatform()", device_get_platform);
    bind("xs/core", "Device", true, "canClose()", device_can_close);
    bind("xs/core", "Device", true, "requestClose()", device_request_close);
    bind("xs/core", "Device", true, "setFullscreen(_)", device_set_fullscreen);

    // Profiler
    bind("xs/core", "Profiler", true, "begin(_)", profiler_begin_section);
    bind("xs/core", "Profiler", true, "end(_)", profiler_end_section);

    // Inspector
    bind("xs/core", "Inspector", true, "text(_)", inspector_text);
    bind("xs/core", "Inspector", true, "treeNode(_)", inspector_tree_node);
    bind("xs/core", "Inspector", true, "treePop()", inspector_tree_pop);
    bind("xs/core", "Inspector", true, "separator()", inspector_separator);
    bind("xs/core", "Inspector", true, "separatorText(_)", inspector_separator_text);
    bind("xs/core", "Inspector", true, "sameLine()", inspector_same_line);
    bind("xs/core", "Inspector", true, "indent()", inspector_indent);
    bind("xs/core", "Inspector", true, "unindent()", inspector_unindent);
    bind("xs/core", "Inspector", true, "spacing()", inspector_spacing);
    bind("xs/core", "Inspector", true, "selectable(_,_)", inspector_selectable);
    bind("xs/core", "Inspector", true, "inputFloat(_,_)", inspector_input_float);
    bind("xs/core", "Inspector", true, "dragFloat(_,_)", inspector_drag_float);
    bind("xs/core", "Inspector", true, "checkbox(_,_)", inspector_checkbox);
    bind("xs/core", "Inspector", true, "beginChild(_,_,_,_)", inspector_begin_child);
    bind("xs/core", "Inspector", true, "endChild()", inspector_end_child);
    bind("xs/core", "Inspector", true, "collapsingHeader(_)", inspector_collapsing_header);
    bind("xs/core", "Inspector", true, "dragFloat2_(_,_,_)", inspector_drag_float2);
}
