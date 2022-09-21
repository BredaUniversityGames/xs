#include "configuration.h"
#include "data.h"

using namespace xs::configuration;

int xs::configuration::width()
{
	int width = (int)xs::data::get_number("Width", xs::data::type::system);
	return width != 0 ? width : 640;
}

int xs::configuration::height()
{
	int height = (int)data::get_number("Height", data::type::system);
	return height != 0.0 ? height : 360;
}

std::string xs::configuration::title()
{
	auto title = data::get_string("Title", data::type::system);
	return !title.empty() ? title : "xs - window";
}

bool xs::configuration::fullscreen()
{
	return false;
}

int xs::configuration::multiplier()
{
	int mult = (int)data::get_number("Multiplier", data::type::system);
	return mult != 0 ? mult : 1;
}
