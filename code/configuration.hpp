#pragma once
#include <string>

namespace xs::configuration
{
	int width();
	int height();
	std::string title();

	/// Multiplier for the window from the game's native resolution
	int multiplier();

	/// Snap sprites to pixel positions
	bool snap_to_pixels();

	/// Points are used on screens with HDPI
	bool window_size_in_points();

	/// Parameters for transforming canvas coordinates to game coordinates.
	/// The canvas can represent anything, such as the game window or a touch pad.
	struct scale_parameters
	{
		float multiplier;
		int xmin, ymin, xmax, ymax;
	};

	/// Computes parameters for transforming canvas coordinates to game coordinates.
	scale_parameters get_scale_to_game(int input_width, int input_height);

	/// Transforms canvas coordinates to game coordinates using the given scaling parameters.
	void scale_to_game(int input_x, int input_y, const scale_parameters& params, float& game_x, float& game_y);
}
