#pragma once

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

	void initialize();
	void shutdown();
	void update(double dt);
}