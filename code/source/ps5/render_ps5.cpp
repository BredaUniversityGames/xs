#include <render.h>
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(PLATFORM_PS5) || defined(PLATFORM_SWITCH)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wreturn-type"
	#define STB_IMAGE_IMPLEMENTATION
	#include <stb/stb_image.h>
	#include <stb/stb_easy_font.h>
#pragma clang diagnostic pop
#elif defined(PLATFORM_PC)
	#define STB_IMAGE_IMPLEMENTATION
	#include <stb/stb_image.h>
	#include <stb/stb_easy_font.h>
#endif


#include <configuration.h>
#include <fileio.h>
#include <log.h>

using namespace glm;

namespace xs::render::internal
{
	struct image
	{
		//GLuint	gl_id = 0;
		int		width = -1;
		int		height = -1;
		int		channels = -1;
	};

	int width = -1;
	int height = -1;
}

using namespace xs::render::internal;

void xs::render::initialize()
{
	width = configuration::width;
	height = configuration::height;
}

void xs::render::shutdown()
{
}

void xs::render::render()
{
}

void xs::render::clear()
{
}

int xs::render::load_image(const std::string& image_file)
{
	return -1;
}

void xs::render::image(int image_id, double x, double y)
{
}

void xs::render::begin(primitive p)
{
}

void xs::render::vertex(double x, double y)
{
}

void xs::render::end()
{
}

void xs::render::color(double r, double g, double b, double a)
{
}

void xs::render::line(double x0, double y0, double x1, double y1)
{
}

void xs::render::text(const std::string& text, double x, double y, double size)
{
}

void xs::render::poly(double x, double y, double radius, int sides)
{
}

void xs::render::rect(double x, double y, double size_x, double size_y, double rotation)
{
}
