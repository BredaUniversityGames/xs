#include "render.h"
#include "configuration.h"
#include "profiler.h"

namespace xs::render::internal
{
}

using namespace xs;
using namespace xs::render::internal;

void xs::render::initialize()
{
}

void xs::render::shutdown()
{
}

void xs::render::render()
{
    XS_PROFILE_SECTION("xs::render::render");
}

void xs::render::clear()
{
}

/*
void xs::render::internal::create_texture_with_data(xs::render::internal::image& img, uchar* data)
{
}
 */

void xs::render::begin(primitive p)
{
}

void xs::render::vertex(double x, double y)
{
}

void xs::render::end()
{
}

void xs::render::set_color(double r, double g, double b, double a)
{
}

void xs::render::set_color(color c)
{
}

void xs::render::line(double x0, double y0, double x1, double y1)
{}

void xs::render::text(const std::string& text, double x, double y, double size)
{}
