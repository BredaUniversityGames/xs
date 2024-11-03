#pragma once
#include <string>
#include "color.h"

namespace xs::render
{	
	/// Bit flags for sprite rendering
	enum sprite_flags
	{
		bottom		= 1 << 1,
		top			= 1 << 2,
		center_x	= 1 << 3,
		center_y	= 1 << 4,
		flip_x		= 1 << 5,
		flip_y		= 1 << 6,
		overlay		= 1 << 7,
		center		= center_x | center_y
	};

	/// Stastistics for the rendering to share with the engine
	struct stats
	{
		int draw_calls = 0;
		int sprites = 0;
		int textures = 0;
	};

	/// Initialize the rendering systems
	void initialize();

	/// Shutdown the rendering systems and (hopefully) free all resources
	void shutdown();

	/// Render the current frame
	void render();

	///  Clear the render queue from the previous frame
	void clear();

	/// (Hot) reload all images in use
	void reload_images();

	/// Set the offset for the rendering - used for camera movement
	void set_offset(double x, double y);

	/// Load an image from a file (png or jpg)
	int load_image(const std::string& image_file);

	/// Load a font from a file (ttf)
	int load_font(const std::string& font_file, double size);

	/// Get the width of an image
	int get_image_width(int image_id);

	/// Get the height of an image
	int get_image_height(int image_id);	

	/// Create a sprite from an image with given texture coordinates
	int create_sprite(int image_id, double x0, double y0, double x1, double y1);

	/// Create a shape with vertex and index data
	int create_shape(
		int image_id,
		const float* positions,
		const float* texture_coordinates,
		unsigned int vertex_count,
		const unsigned short* indices,
		unsigned int index_count);

	/// Destroy a sprite
	void destroy_shape(int shape_id);

	/// Render a sprite with given position, size, rotation and colors
	void sprite(
		int sprite_id,
		double x,
		double y,		
		double z,
		double size,
		double rotation,
		color mutiply,
		color add,
		unsigned int flags);

	/// Render text with given font, position, size and colors
	void text(
		int font_id,
		const std::string& text,
		double x,
		double y,
		double z,
		color mutiply,
		color add,
		unsigned int flags);

	/// Get the statistics for the rendering
	stats get_stats();
	
	/// Primitives for debugging
	enum class dbg_primitive { lines, triangles, none };

	/// Begin a new primitive
	void dgb_begin(dbg_primitive p);

	/// Add a vertex to the current primitive
	void dbg_vertex(double x, double y);

	/// Set the color for the current primitive
	void dbg_color(color c);

	/// End the current primitive
	void dbg_end();

	/// Draw a line
	void dbg_line(double x0, double y0, double x1, double y1);

	/// Draw some text made up of squares
	void dbg_text(const std::string& text, double x, double y, double size);
}
