#include "device.h"
#include "configuration.h"

void xs::device::get_render_scale(float& multiplier, int& xmin, int& ymin, int& xmax, int& ymax)
{
	// Calculate how to fit the buffer onto the screen.
	// We want to fill the screen as much as possible while perserving the game's aspect ratio.
	int deviceWidth = get_width(), deviceHeight = get_height();
	int gameWidth = xs::configuration::width, gameHeight = xs::configuration::height;

	// Note: in windowed mode, this will just be configuration::multiplier again. 
	// In fullscreen mode when the game and screen resolution do not match, we need to compute it here.
	multiplier = std::min((float)deviceWidth / gameWidth, (float)deviceHeight / gameHeight);
	xmin = static_cast<int>((deviceWidth - gameWidth * multiplier) / 2);
	xmax = deviceWidth - xmin;
	ymin = static_cast<int>((deviceHeight - gameHeight * multiplier) / 2);
	ymax = deviceHeight - ymin;
}

void xs::device::screen_to_game(int screen_x, int screen_y, float& game_x, float& game_y)
{
	float multiplier; int xmin, ymin, xmax, ymax;
	get_render_scale(multiplier, xmin, ymin, xmax, ymax);

	game_x = (screen_x - xmin) / multiplier - xs::configuration::width / 2;
	game_y = xs::configuration::height / 2 - (screen_y - ymin) / multiplier;
}
