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

extern "C" {
#include "wren_opt_random.h"
}

using namespace std;

namespace xs::script::internal
{
	WrenVM* vm = nullptr;
	WrenHandle* game_class = nullptr;
	WrenHandle* init_method = nullptr;
	WrenHandle* update_method = nullptr;
	std::unordered_map<size_t, WrenForeignMethodFn> foreign_methods;
	std::unordered_map<size_t, std::string> modules;
	bool initialized = false;
	const char* main;
	bool error = false;
	
	void writeFn(WrenVM* vm, const char* text)
	{
		if (strcmp(text, "\n") == 0)
			return;

		static auto magenta = "\033[35m";
		static auto reset = "\033[0m";
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);		
		const auto time = std::put_time(&tm, "[%Y-%m-%d %T.%e0] ");
		//std::cout << time << "[" << magenta << "script" << reset << "] " << text << endl;
		std::cout << "[" << magenta << "script" << reset << "] " << text << endl;
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
		const char* module,
		const char* class_name,
		bool is_static,
		const char* signature)
	{
		const string dot = is_static ? "::" : ".";
		const string concat = string(module) + "::" + string(class_name) + dot + string(signature);
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
			const auto filename = "[games]/shared/modules/" + string(name) + ".wren";
			const auto id = hash<string>{}(filename);
			modules[id] = xs::fileio::read_text_file(filename);
			res.source = modules[id].c_str();
		}
		return res;
	}
	
	void call_init() {}
}

using namespace xs::script::internal;

void xs::script::initialize(const char* main)
{
	initialized = false;
	error = false;

	if (main != nullptr)
		internal::main = main;

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

	vm = wrenNewVM(&config);	
	const auto module = "main";
	const std::string script_file = fileio::read_text_file(internal::main);
	const WrenInterpretResult result = wrenInterpret(vm, module, script_file.c_str());
	
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
			log::info(string("Script compile success. ") + pr);
		} break;
	}

	if (initialized && !error)
	{
		wrenEnsureSlots(vm, 1);										// Make sure there at least one slot
		wrenGetVariable(vm, "main", "Game", 0);						// Grab a handle to the Game class
		game_class = wrenGetSlotHandle(vm, 0);
		wrenSetSlotHandle(vm, 0, game_class);						// Put Game class in slot 0
		init_method = wrenMakeCallHandle(vm, "init()");
		update_method = wrenMakeCallHandle(vm, "update(_)");

		call_init();
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
		wrenFreeVM(vm);
		vm = nullptr;
	}

	foreign_methods.clear();
	modules.clear();
}

void xs::script::update(double dt)
{
	if (initialized)
	{
		wrenEnsureSlots(vm, 2);
		wrenSetSlotHandle(vm, 0, game_class);
		wrenSetSlotDouble(vm, 1, dt);
		wrenCall(vm, update_method);
	}
}

bool xs::script::has_error()
{
	return error;
}

void xs::script::bind(
	const char* module,
	const char* class_name,
	bool is_static,
	const char* signature,
	WrenForeignMethodFn func)
{
	const auto id = get_method_id(module, class_name, is_static, signature);
	foreign_methods[id] = func;
}

