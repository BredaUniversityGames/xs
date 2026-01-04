#pragma once
#include <string>

typedef struct WrenVM WrenVM;
typedef void (*WrenForeignMethodFn)(WrenVM* vm);
typedef void (*WrenFinalizerFn)(void* data);

namespace xs::script
{
	void configure();
	void initialize();
	void shutdown();
	void update(double dt);
	void render();
	void ec_inspect(bool& open);
	bool has_error();
	void clear_error();
	void bind_api();
	void bind(	
		const std::string& module,
		const std::string& class_name,
		bool is_static,
		const std::string& signature,
		WrenForeignMethodFn func);
	void bind(
		const std::string& module,
		const std::string& class_name,
		WrenForeignMethodFn allocate_fn,
		WrenFinalizerFn finalize_fn = NULL);
    size_t get_bytes_allocated();
}
