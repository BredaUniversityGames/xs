#pragma once
#include <string>

namespace xs::render
{	
	void initialize();
	void shutdown();	
	void render();
	void clear();

	int load_image(const std::string& image_file);
	void image(int image_id, double x, double y);	// Not implemented yet

	enum class primitive { lines, triangles, none };
	void begin(primitive p);
	void vertex(double x, double y);
	void color(double r, double g, double b, double a);
	void end();	
	void line(double x0, double y0, double x1, double y1);
	void text(const std::string& text, double x, double y, double size);


	void rect(double x, double y, double size_x, double size_y, double rotation);	// deprecated, will be removed
	void poly(double x, double y, double radius, int sides); // deprecated, will be removed
}
