#pragma once

typedef struct WrenVM WrenVM;
typedef void (*WrenForeignMethodFn)(WrenVM* vm);

namespace xs::script
{
	void initialize(const char* main);
	void shutdown();
	void update(double dt);
	bool has_error();
	void clear_error();
	void bind_api();
	void bind(
		const char* module,
		const char* class_name,
		bool is_static,
		const char* signature,
		WrenForeignMethodFn func);
}
