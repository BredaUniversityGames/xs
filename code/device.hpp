#pragma once

namespace xs::device
{
	void initialize();
	void shutdown();
	void begin_frame();
	void end_frame();
	void poll_events();
	void start_frame();
	bool should_close();
	int get_width();
	int get_height();
    void set_window_size(int w, int h);
	double hdpi_scaling();
    
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

	/// <summary>
	/// Returns whether the game is allowed to close itself on the target platform.
	/// Use this to (for example) show/hide a quit button without having to ask for the platform itself.
	/// </summary>
	/// <returns>true if the target platform allows the game to close itself; false otherwise.</returns>
	bool can_close();

	/// <summary>
	/// Tries to let the game close itself in the upcoming frame.
	/// </summary>
	/// <returns>true if the request was successful; false otherwise (for example if the platform does not allow it).</returns>
	bool request_close();

	/// <summary>
	/// Sets fullscreen mode on or off. Maintains aspect ratio with black bars.
	/// </summary>
	void set_fullscreen(bool fullscreen);

    bool get_fullscreen();
	
	/// <summary>
	/// Toggles whether the application window should stay on top of other windows.
	/// </summary>
	bool toggle_on_top();
}
