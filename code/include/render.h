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
			//struct { uchar r, g, b, a; };
			struct { uchar a, b, g, r; };
			uint32_t integer_value;
		};
	};

	enum class sprite_anchor
	{
		bottom = 0,
		center = 1
	};

	void initialize();
	void shutdown();	
	void render();
	void clear();

	int load_image(const std::string& image_file);
	int create_sprite(int image_id, double x0, double y0, double x1, double y1);
	void render_sprite(int image_id, double x, double y, sprite_anchor anchor);

	enum class primitive { lines, triangles, none };
	void begin(primitive p);
	void vertex(double x, double y);
	void set_color(double r, double g, double b, double a);
	void set_color(color c);
	void end();	
	void line(double x0, double y0, double x1, double y1);
	void text(const std::string& text, double x, double y, double size);
}
