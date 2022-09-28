#pragma once

namespace xs::device
{
	void initialize();
	void shutdown();
	void swap_buffers();
	void poll_events();
	bool should_close();
	int get_width();
	int get_height();

	enum platform 
	{
		PC = 0,
		PS5,
		SWITCH,
		PLATFORM_UNKNOWN
	};

	/// <summary>
	/// Gets the target platform of the program.
	/// Use it as an API function to let external scripts ask for the current platform at run-time.
	/// </summary>
	platform get_platform();
}
