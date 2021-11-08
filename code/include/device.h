#pragma once

namespace xs::device
{
	void initialize();
	void shutdown();
	void swap_buffers();
	void poll_events();
	bool should_close();
}
