#pragma once
#include <string>

typedef struct WrenVM WrenVM;
typedef void (*WrenForeignMethodFn)(WrenVM* vm);

namespace xs::script
{
	void configure();
	void initialize();
	void shutdown();
	void update(double dt);
	void render();
	bool has_error();
	void clear_error();
	void bind_api();
	void bind(	// TODO: use std::string
		const char* module,
		const char* class_name,
		bool is_static,
		const char* signature,
		WrenForeignMethodFn func);
}
