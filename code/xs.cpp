#include "xs.hpp"

using namespace xs;
#include "fileio.hpp"
#include "device.hpp"
#include "input.hpp"
#include "log.hpp"
#include "render.hpp"
#include "script.hpp"
#include "audio.hpp"
#include "account.hpp"
#include "data.hpp"
#include "inspector.hpp"
#include "packager.hpp"
#include "tools.hpp"

static run_mode s_run_mode = run_mode::development;

void xs::set_run_mode(run_mode mode)
{
	s_run_mode = mode;
}

run_mode xs::get_run_mode()
{
	return s_run_mode;
}

void xs::initialize(const std::string& game_path)
{
	log::initialize();
	account::initialize();
	fileio::initialize(game_path);
	data::initialize();
	script::configure();

	auto mode = get_run_mode();

	// Packaging mode: skip all windowing/rendering/game systems
	if (mode != run_mode::packaging)
	{
		device::initialize();
		render::initialize();
		input::initialize();
		audio::initialize();
	}

	// Inspector in development and packaged modes (packaged shows limited UI)
	// In release builds, inspector is not compiled anyway
	if (mode != run_mode::packaging)
	{
		inspector::initialize();
	}

	// Script VM only needed for running games
	if (mode != run_mode::packaging)
	{
		script::initialize();
	}
}

void xs::shutdown()
{
	auto mode = get_run_mode();

	// Inspector in development and packaged modes
	if (mode != run_mode::packaging)
	{
		inspector::shutdown();
	}

	// Shutdown systems that were initialized
	if (mode != run_mode::packaging)
	{
		audio::shutdown();
		input::shutdown();
		render::shutdown();
		device::shutdown();
		script::shutdown();
	}

	account::shutdown();
	data::shutdown();
}

void xs::update(double dt)
{
	auto mode = get_run_mode();

	// Packaging mode shouldn't reach here, but safety check
	if (mode == run_mode::packaging)
	{
		return;
	}

	device::poll_events();
	input::update(dt);

	// Development mode: respect inspector pause state
	// Packaged mode: always update (no pause functionality)
	bool should_update = (mode == run_mode::development) ? !inspector::paused() : true;

	if (should_update)
	{
		render::clear();
		script::update(dt);
		audio::update(dt);
		script::render();
	}

	device::begin_frame();
	render::render();

	// Inspector in both development and packaged modes
	// Inspector will internally show limited UI based on run_mode
	if (mode != run_mode::packaging)
	{
		inspector::render(dt);

		// Restart only available in development mode
		if (mode == run_mode::development && inspector::should_restart())
		{
			shutdown();
			initialize();
		}
	}

	device::end_frame();
}