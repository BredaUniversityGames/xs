#include "xs.hpp"
#include "device.hpp"
#include <chrono>
#include "main.hpp"

// CLI support for PC and Mac only
#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
#include "../external/argparse/argparse.hpp"
#include <iostream>
#include <filesystem>
#endif

using namespace std;


#if defined(PLATFORM_PC)
#define DWORD unsigned int
extern "C"
{
    // Force Nvidia GPU if available
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

int main(int argc, char* argv[])
{
	return  xs::main(argc, argv);	
}
#endif


int xs::main(int argc, char* argv[])
{
#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
	// Parse command line arguments
	argparse::ArgumentParser program("xs", "0.2.0");
	program.add_description("xs game engine");

	// Run subcommand - runs a project folder or .xs package
	argparse::ArgumentParser run_cmd("run");
	run_cmd.add_description("Run a game project or packaged .xs file");
	run_cmd.add_argument("path")
		.help("Path to game project folder or .xs package file")
		.default_value(std::string("."));

	// Package subcommand - creates a .xs package from a project
	argparse::ArgumentParser package_cmd("package");
	package_cmd.add_description("Package a game project into a .xs file");
	package_cmd.add_argument("input")
		.help("Path to game project folder");
	package_cmd.add_argument("output")
		.help("Output .xs package file path (optional)")
		.default_value(std::string(""))
		.nargs(argparse::nargs_pattern::optional);

	// Add subcommands to main program
	program.add_subparser(run_cmd);
	program.add_subparser(package_cmd);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	}

	// Determine which subcommand was used
	std::string game_path;
	if (program.is_subcommand_used("run")) {
		std::string path = run_cmd.get<std::string>("path");

		// Auto-detect if path is a .xs package or project folder
		std::filesystem::path fs_path(path);
		if (fs_path.extension() == ".xs") {
			xs::set_run_mode(xs::run_mode::packaged);
			// TODO: Handle .xs package loading (future work)
			game_path = path;
		} else {
			xs::set_run_mode(xs::run_mode::development);
			game_path = path;
		}
	}
	else if (program.is_subcommand_used("package")) {
		xs::set_run_mode(xs::run_mode::packaging);
		std::string input = package_cmd.get<std::string>("input");
		std::string output = package_cmd.get<std::string>("output");

		// TODO: Implement packaging (Step 6)
		std::cout << "Packaging: " << input << " -> " << output << std::endl;
		std::cout << "Package command not yet implemented" << std::endl;
		return 0;
	}
	else {
		// No subcommand - default to running current directory
		xs::set_run_mode(xs::run_mode::development);
		game_path = ".";
	}
#else
	// Console platforms - always run in packaged mode
	xs::set_run_mode(xs::run_mode::packaged);
	std::string game_path;
#endif

	xs::initialize(game_path);
    auto prev_time = chrono::high_resolution_clock::now();
    while (!device::should_close())
    {
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = current_time - prev_time;
        prev_time = current_time;
        auto dt = std::chrono::duration<double>(elapsed).count();
        dt = std::min(dt, 0.03333);
        xs::update((float)dt);
    }
	xs::shutdown();
	return 0;
}
