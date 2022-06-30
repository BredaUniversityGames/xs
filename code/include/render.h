#pragma once
#include <string>

namespace xs::render
{	
	using  uchar = unsigned char;
	struct color
	{ 
		union
		{
			uchar rgba[4];
			struct { uchar a, b, g, r; };
			//struct { uchar r, g, b, a; };
			uint32_t integer_value;
		};
	};

	/*
	enum class sprite_anchor
	{
		bottom = 0,
		center = 1
	};
	*/

	enum sprite_flags
	{
		bottom = 1 << 1,
		center = 1 << 2,
		flip_x = 1 << 3,
		flip_y = 1 << 4,
	};

	void initialize();
	void shutdown();	
	void render();
	void clear();
	void set_offset(double x, double y);

	int load_image(const std::string& image_file);
	int load_font(const std::string& font_file, double size);

	// int get_image_width(int image_id);
	// int get_image_height(int image_id);
	
	int create_sprite(int image_id, double x0, double y0, double x1, double y1);
	void render_sprite(
		int sprite_id,
		double x,
		double y,		
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

	enum class primitive { lines, triangles, none };
	void begin(primitive p);
	void vertex(double x, double y);
	void set_color(double r, double g, double b, double a);
	void set_color(color c);
	void end();	
	void line(double x0, double y0, double x1, double y1);
	void text(const std::string& text, double x, double y, double size);
}
