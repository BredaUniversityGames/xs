#include "xs.h"

using namespace xs;
#include "fileio.h"
#include "device.hpp"
#include "input.hpp"
#include "log.h"
#include "render.h"
#include "script.hpp"
#include "audio.h"
#include "account.h"
#include "data.h"
#include "inspector.h"
#include "cooker.hpp"
#include "tools.h"
#include "editor.h"


void xs::initialize()
{
	log::initialize();
	editor::initialize();
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

void xs::update(float dt)
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
		xs::shutdown();
		xs::initialize();
	}
}