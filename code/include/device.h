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

	void get_render_scale(float& multiplier, int& xmin, int& ymin, int& xmax, int& ymax);
	void screen_to_game(int screen_x, int screen_y, float& game_x, float& game_y);
}
