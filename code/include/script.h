#pragma once

typedef struct WrenVM WrenVM;
typedef void (*WrenForeignMethodFn)(WrenVM* vm);

namespace xs
{
	enum class result {
		success,
		fail,
	};
}

namespace xs::script
{
	result initialize(const char* main);
	void shutdown();
	void update(double dt);
	bool has_error();
	void bind_api();
	void bind(
		const char* module,
		const char* class_name,
		bool is_static,
		const char* signature,
		WrenForeignMethodFn func);
}
