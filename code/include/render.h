#pragma once
#include <string>

namespace xs::render
{	
	using uchar = unsigned char;
	struct color
	{ 
		union
		{
			uchar rgba[4];
			struct { uchar a, b, g, r; };
			uint32_t integer_value;
		};
	};

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

	void initialize();
	void shutdown();	
	void render();
	void clear();
	void reload();
	void reload_images();
	void set_offset(double x, double y);
	void set_3d_projection(const float* projection);
	void set_3d_view(const float* view);

	int load_image(const std::string& image_file);
	int load_font(const std::string& font_file, double size);
	int load_mesh(const std::string& mesh_file);
	int get_image_width(int image_id);
	int get_image_height(int image_id);
	
	int create_sprite(int image_id, double x0, double y0, double x1, double y1);
	int create_mesh(
		const unsigned int* indices,
		unsigned int index_count,
		const float* positions,
		const float* normals,
		const float* texture_coordinates,
		const float* colors,
		unsigned int vertex_count);

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
		color mutiply,
		color add,
		unsigned int flags);

	void render_mesh(
		int mesh_id,
		int image_id,
		const float* transform,
		color mutiply,
		color add);

	enum class primitive { lines, triangles, none };
	void begin(primitive p);
	void vertex(double x, double y);
	void set_color(color c);
	void end();	
	void line(double x0, double y0, double x1, double y1);
	void text(const std::string& text, double x, double y, double size);
}
