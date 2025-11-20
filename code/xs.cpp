#include "xs.hpp"
#include "fileio.hpp"
#include "device.hpp"
#include "input.hpp"
#include "log.hpp"
#include "render.hpp"
#include "script.hpp"
#include "audio.hpp"
#include "simple_audio.hpp"
#include "account.hpp"
#include "data.hpp"
#include "inspector.hpp"
#include "packager.hpp"
#include "version.hpp"
#include <chrono>

// CLI support for PC and Mac only
#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
#include "../external/argparse/argparse.hpp"
#include "packager.hpp"
#include "fileio.hpp"
#include <iostream>
#include <filesystem>
#endif

using namespace xs;
using namespace std;

static run_mode s_run_mode = run_mode::development;

void xs::set_run_mode(run_mode mode)
{
	s_run_mode = mode;
}

run_mode xs::get_run_mode()
{
	return s_run_mode;
}

int xs::dispatch(int argc, char* argv[])
{
#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
	// Parse command line arguments
	argparse::ArgumentParser program("xs", version::get_version_string(false, true, true).c_str());
	program.add_description("xs game engine");

	// Run subcommand - runs a project folder or .xs package
	argparse::ArgumentParser run_cmd("run");
	run_cmd.add_description("Run a game project or packaged .xs file");
	run_cmd.add_argument("path")
		.help("Path to game project folder or .xs package file")
		.default_value(std::string("."));

	// Run subcommand - runs a project folder or .xs package
	argparse::ArgumentParser version_cmd("version");
	version_cmd.add_description("Get the version of xs");

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
	program.add_subparser(version_cmd);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << err.what() << '\n';
		std::cerr << program;
		return 1;
	}

	std::string game_path;
	if (program.is_subcommand_used("version"))
	{
		printf("%s", version::get_version_string(false, true, true).c_str());
		return 0;
	}
	
	if (program.is_subcommand_used("run")) {
		std::string path = run_cmd.get<std::string>("path");

		// Auto-detect if path is a .xs package or project folder
		std::filesystem::path fs_path(path);
		if (fs_path.extension() == ".xs") {
			xs::set_run_mode(xs::run_mode::packaged);
			game_path = path;
		}
		else {
			xs::set_run_mode(xs::run_mode::development);
			game_path = path;
		}
		return xs::main(game_path);
	}
	else if (program.is_subcommand_used("package")) {
		xs::set_run_mode(xs::run_mode::packaging);
		std::string input = package_cmd.get<std::string>("input");
		std::string output = package_cmd.get<std::string>("output");
		return package(input, output);
	}
	else {
		// No subcommand - show usage/help and quit with non-zero exit to indicate user/usage error
		std::cout << program;
		return 2;
	}
#endif

	return 0;
}

int xs::package(std::string& input, std::string& output)
{
	// Initialize fileio with input path to set up [game] wildcard
	// This is needed for packager to find the files
	log::initialize();
	fileio::initialize(input);
	data::initialize();
	script::configure();

	// Generate output path if not provided
	if (output.empty()) {
		std::filesystem::path input_path(input);
		std::string folder_name = input_path.filename().string();
		if (folder_name.empty()) {
			// Handle case like "." or paths ending with /
			folder_name = input_path.parent_path().filename().string();
		}
		if (folder_name.empty()) {
			folder_name = "game";
		}
		output = folder_name + ".xs";
	}

	xs::log::info("Packaging: {} -> {}", input, output);

	// Create the package
	bool success = xs::packager::create_package(output);
	if (success)
		xs::log::info("Package created successfully: {}", output);
	else
		xs::log::error("Failed to create package");

	data::shutdown();

	return 0;
}

void xs::initialize(const std::string& game_path)
{
	log::initialize();
	account::initialize();
	fileio::initialize(game_path);
	data::initialize();
	script::configure();
	device::initialize();
	render::initialize();
	input::initialize();
	audio::initialize();
	simple_audio::initialize();
	inspector::initialize();
	script::initialize();
}

void xs::shutdown()
{
	inspector::shutdown();
	simple_audio::shutdown();
	audio::shutdown();
	input::shutdown();
	render::shutdown();
	device::shutdown();
	script::shutdown();
	account::shutdown();
	data::shutdown();
}

void xs::update(double dt)
{
	device::poll_events();
	input::update(dt);

	if (!inspector::paused())
	{
		render::clear();
		script::update(dt);
		audio::update(dt);
		simple_audio::update(dt);
		script::render();
	}

	device::begin_frame();
	render::render();
	inspector::render(dt);
	device::end_frame();
}

int xs::main(const std::string& game_path)
{
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

