#pragma once
#include <string>

namespace xs::configuration
{
	int width();
	int height();
	std::string title();
	bool fullscreen(); //TODO: Not suppported yet

	/// Multiplier for the window from the game's native resolution
	int multiplier();

	/// Put the window on top
	bool on_top();

	/// Snap sprites to pixel positions
	bool snap_to_pixels();

	/// Point are used on screens with HDPI
	bool window_size_in_points();

	/// <summary>
	/// Parameters for transforming canvas coordinates to game coordinates.
	/// The canvas can represent anything, such as the game window or a touch pad.
	/// </summary>
	struct scale_parameters
	{
		float multiplier;
		int xmin, ymin, xmax, ymax;
	};

	/// <summary>
	/// Computes parameters for transforming canvas coordinates to game coordinates.
	/// </summary>
	/// <param name="input_width">The width of the canvas for which we want to compute scaling parameters.</param>
	/// <param name="input_height">The height of the canvas for which we want to compute scaling parameters.</param>
	/// <returns>A scale_parameters object describing how to map an "input_width X input_height" canvas to the game's coordinate system.</returns>
	scale_parameters get_scale_to_game(int input_width, int input_height);

	/// <summary>
	/// Transforms canvas coordinates to game coordinates, using the given scaling parameters.
	/// Compute these scaling parameters beforehand using get_scale_to_game().
	/// </summary>
	/// <param name="input_x">The x coordinate to transform.</param>
	/// <param name="input_y">The y coordinate to transform.</param>
	/// <param name="params">The parameters for scaling coordinates on the input canvas to game coordinates.</param>
	/// <param name="game_x">[Out] The corresponding x coordinate in game coordinates.</param>
	/// <param name="game_y">[Out] The corresponding y coordinate in game coordinates.</param>
	void scale_to_game(int input_x, int input_y, const scale_parameters& params, float& game_x, float& game_y);
}
