#include "configuration.hpp"
#include "data.hpp"

using namespace xs::configuration;

int xs::configuration::width()
{
	return (int)xs::data::get_number("Device.Width", xs::data::type::project, 640.0);
}

int xs::configuration::height()
{
	return (int)data::get_number("Device.Height", data::type::project, 360.0);
}

std::string xs::configuration::title()
{
	return data::get_string("Device.Title", data::type::project, "xs");
}

int xs::configuration::multiplier()
{
	return (int)data::get_number("Device.Multiplier", data::type::project, 1.0);
}

bool xs::configuration::snap_to_pixels()
{
    return data::get_bool("Render.SnapToPixels", data::type::project);
}

bool xs::configuration::window_size_in_points()
{
    return data::get_bool("Device.WindowSizeInPoints", data::type::project);
}


scale_parameters xs::configuration::get_scale_to_game(int input_width, int input_height)
{
	// Calculate how to fit the buffer onto the screen.
	// We want to fill the screen as much as possible while perserving the game's aspect ratio.
	int gameWidth = width(), gameHeight = height();

	scale_parameters result;

	// Note: in windowed mode, this will just be configuration::multiplier again. 
	// In fullscreen mode when the game and screen resolution do not match, we need to compute it here.
	result.multiplier = std::min((float)input_width / gameWidth, (float)input_height / gameHeight);
	result.xmin = static_cast<int>((input_width - gameWidth * result.multiplier) / 2);
	result.xmax = input_width - result.xmin;
	result.ymin = static_cast<int>((input_height - gameHeight * result.multiplier) / 2);
	result.ymax = input_height - result.ymin;

	return result;
}

void xs::configuration::scale_to_game(int input_x, int input_y, const scale_parameters& params, float& game_x, float& game_y)
{
	game_x = (input_x - params.xmin) / params.multiplier - xs::configuration::width() / 2;
	game_y = xs::configuration::height() / 2 - (input_y - params.ymin) / params.multiplier;
}
