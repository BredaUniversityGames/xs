#pragma once
#include <string>

namespace xs
{
	enum class run_mode
	{
		development,  // Run project folder with editor UI
		packaged,     // Run .xs package (no editor UI)
		packaging     // Package creation (no window/rendering)
	};

	void set_run_mode(run_mode mode);
	run_mode get_run_mode();

	void initialize(const std::string& game_path = "");
	void shutdown();
	void update(double dt);
}