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

void xs::initialize()
{
	log::initialize();
	account::initialize();
	fileio::initialize();
	data::initialize();
	script::configure();
	device::initialize();
	render::initialize();
	input::initialize();
	audio::initialize();
	inspector::initialize();
	script::initialize();
}

void xs::shutdown()
{
	inspector::shutdown();
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
		script::render();
	}
	device::begin_frame();
	render::render();
	inspector::render(dt);
	device::end_frame();
	if (inspector::should_restart())
	{
		shutdown();
		initialize();
	}
}