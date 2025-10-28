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

	int dispatch(int argc, char* argv[]);

	int package(std::string& input, std::string& output);

	void initialize(const std::string& game_path = "");

	void shutdown();

	void update(double dt);

	int main(const std::string& game_path = "");
}