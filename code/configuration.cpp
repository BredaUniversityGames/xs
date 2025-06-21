#include "configuration.hpp"
#include "data.hpp"

using namespace xs::configuration;

int xs::configuration::width()
{
	int width = (int)xs::data::get_number("Width", xs::data::type::project);
	return width != 0 ? width : 640;
}

int xs::configuration::height()
{
	int height = (int)data::get_number("Height", data::type::project);
	return height != 0.0 ? height : 360;
}

std::string xs::configuration::title()
{
	auto title = data::get_string("Title", data::type::project);
	return !title.empty() ? title : "xs - window";
}

bool xs::configuration::fullscreen()
{
	return false;
}

int xs::configuration::multiplier()
{
	int mult = (int)data::get_number("Multiplier", data::type::project);
	return mult != 0 ? mult : 1;
}

bool xs::configuration::on_top()
{
	return data::get_bool("Always on top", data::type::project);
}

bool xs::configuration::snap_to_pixels()
{
    return data::get_bool("Snap to pixels", data::type::project);
}

bool xs::configuration::window_size_in_points()
{
    return data::get_bool("Window size in points", data::type::project);
}

/*
bool xs::configuration::msaa_enabled()
{
	return data::get_bool("MSAA", data::type::project);
}
*/

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
