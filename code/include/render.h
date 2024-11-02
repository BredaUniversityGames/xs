#pragma once
#include <string>
#include "color.h"

namespace xs::render
{	
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

	struct stats
	{
		int draw_calls = 0;
		int sprites = 0;
		int textures = 0;
	};

	void initialize();
	void shutdown();	
	void render();
	void clear();;
	void reload_images();
	void set_offset(double x, double y);
	int load_image(const std::string& image_file);
	int load_font(const std::string& font_file, double size);
	int get_image_width(int image_id);
	int get_image_height(int image_id);	
	int create_sprite(int image_id, double x0, double y0, double x1, double y1);

	int create_shape(
		int image_id,
		const float* positions,
		const float* texture_coordinates,
		unsigned int vertex_count,
		const unsigned short* indices,
		unsigned int index_count);

	void destroy_shape(int sprite_id);

	void render_sprite(
		int sprite_id,
		double x,
		double y,		
		double z,
		double size,
		double rotation,
		color mutiply,
		color add,
		unsigned int flags);

	void render_text(
		int font_id,
		const std::string& text,
		double x,
		double y,
		double z,
		color mutiply,
		color add,
		unsigned int flags);

	enum class primitive { lines, triangles, none };
	void begin(primitive p);
	void vertex(double x, double y);
	void set_color(color c);
	void end();	
	void line(double x0, double y0, double x1, double y1);
	void text(const std::string& text, double x, double y, double size);
	stats get_stats();
}
