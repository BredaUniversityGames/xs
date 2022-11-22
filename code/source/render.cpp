#include "render.h"
#include "render_internal.h"
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "opengl.h"
#include "configuration.h"
#include "fileio.h"
#include "log.h"
#include "tools.h"
#include "device.h"
#include "profiler.h"

// Include STB
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#ifdef PLATFORM_SWITCH
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include <stb/stb_easy_font.h>
#pragma clang diagnostic pop
#else
#include <stb/stb_easy_font.h>
#endif

// Write image for debugging
#define DEBUG_FONT_ATLAS 0
#if DEBUG_FONT_ATLAS
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <stb/stb_image_write.h>
#endif

using namespace glm;

namespace xs::render::internal
{
	std::vector<sprite_queue_entry>	sprite_queue = {};
	std::vector<sprite>				sprites = {};
	std::vector<font_atlas>			fonts = {};
	std::vector<image>				images = {};
	glm::vec2						offset = glm::vec2(0.0f, 0.0f);

	const int FONT_ATLAS_MIN_CHARACTER = 32;
	const int FONT_ATLAS_NR_CHARACTERS = 96;
}

using namespace xs;
using namespace xs::render::internal;

int xs::render::load_font(const std::string& font_file, double size)
{	
	// Find image first
	auto id = std::hash<std::string>{}(font_file + std::to_string(size));
	for (int i = 0; i < fonts.size(); i++)
		if (fonts[i].string_id == id)
			return i;

	int font_id = (int)fonts.size();
	fonts.push_back(font_atlas());
	auto& font = fonts.back();


	const std::string path = fileio::get_path(font_file);
	FILE* file;
#ifdef _WIN32 // defined to 32 and 64
	bool sucess = fopen_s(&file, path.c_str(), "rb") == 0;
#else 
	file = fopen(path.c_str(), "rb");
	bool sucess = file != nullptr;
#endif
	assert(sucess);

	long lSize;
	fseek(file, 0, SEEK_END);		// obtain file size
	lSize = ftell(file);
	rewind(file);

	// Allocate memory to contain the whole file
	font.buffer = static_cast<unsigned char*>(malloc(sizeof(char) * lSize));
	// read the font into that data
	const size_t result = fread(reinterpret_cast<void*>(font.buffer), 1, lSize, file);

	if (result)
		log::info("Number of characters read from font = {}\n", result);
	fclose(file);

	// Get font info
	if (!stbtt_InitFont(&font.info, font.buffer, 0))
		log::info("initializing font has failed\n");
	
	// calculate by what factor to scale the font at render time
	int ascent;
	int descent;
	int lineGap;
	stbtt_GetFontVMetrics(&font.info, &ascent, &descent, &lineGap);
	font.scale = (size / double(ascent - descent));

	// calculate how big the font atlas png should be
	int requiredPixels = (int)size * (int)size * FONT_ATLAS_NR_CHARACTERS;
	auto dimension = tools::next_power_of_two((uint32)sqrt(requiredPixels));

	image img;
	int val = ascent - descent;
	img.width = dimension;
	img.height = dimension;
	auto bitmap = static_cast<unsigned char*>(malloc(img.width * img.height));

	// pack font
	font.packed_chars.resize(96);
	
	// pack font
	stbtt_pack_context pc;
	stbtt_PackBegin(&pc, bitmap, img.width, img.height, 0, 2, nullptr);
	font.packed_chars.resize(96);
	stbtt_PackFontRange(&pc, font.buffer, 0, (float)size, FONT_ATLAS_MIN_CHARACTER, FONT_ATLAS_NR_CHARACTERS, &font.packed_chars[0]);
	stbtt_PackEnd(&pc);

	// make the image monochrome; preserve alpha
	auto n = img.width * img.height;
	auto rgba = new color[n];
	for (int i = 0; i < n; i++) {
		color c;
		c.rgba[0] = 255;	// Channels are flipped here :D
		c.rgba[1] = 255;
		c.rgba[2] = 255;
		c.rgba[3] = bitmap[i];
		rgba[i] = c;
	}


	img.channels = 4;
	img.string_id = id;
	create_texture_with_data(img, (uchar*)rgba);

	int image_id = (int)images.size();
	images.push_back(img);
	font.image_id = image_id;
	font.string_id = id;

#if DEBUG_FONT_ATLAS
	std::string filename = std::string("FontAtlas-") + std::to_string(size) + ".png";
	stbi_write_png(filename.c_str(), img.width, img.height, 4, rgba, 0);
#endif
	free(bitmap);

	return font_id;
}

int xs::render::get_image_height(int image_id)
{
	auto& img = images[image_id];
	return img.height;
}

int xs::render::get_image_width(int image_id)
{
	auto& img = images[image_id];
	return img.width;
}


void xs::render::render_text(
	int font_id,
	const std::string& text,
	double x,
	double y,
	color multiply,
	color add,
	unsigned int flags)
{
	auto& font = fonts[font_id];
	auto& img = images[font.image_id];

	double begin = x;	
	auto last_idx = sprite_queue.size();
	for (size_t i = 0; i < text.size(); i++)
	{
		const int charIndex = text[i] - FONT_ATLAS_MIN_CHARACTER;

		stbtt_aligned_quad quad;
		float tx = 0;
		float ty = 0;

		stbtt_GetPackedQuad(&font.packed_chars[0], img.width, img.height, charIndex, &tx, &ty, &quad, 0);
		const int glyphIndex = stbtt_FindGlyphIndex(&font.info, text[i]);

		int advance_i = 0, bearing_i = 0;
		stbtt_GetGlyphHMetrics(&font.info, glyphIndex, &advance_i, &bearing_i);
		double advance = advance_i * font.scale;
		double bearing = bearing_i * font.scale;
		tx *= (float)font.scale;
		ty *= (float)font.scale;

		// Kerning to next letter
		float kerning = 0.0f;
		if (i + 1 < text.size())
		{
			const auto nextCodepoint = text[i + 1];
			const auto next = stbtt_FindGlyphIndex(&font.info, nextCodepoint);
			auto kern = stbtt_GetGlyphKernAdvance(&font.info, glyphIndex, next);
			kerning = (float)kern * (float)font.scale;
		}

		auto sprite = create_sprite(font.image_id, quad.s0, quad.t0, quad.s1, quad.t1);
		render_sprite(sprite, begin + bearing, y - quad.y1, 100000, 1, 0, multiply, add, 0);

		begin += advance + kerning;		
	}


	if (tools::check_bit_flag_overlap(flags, xs::render::sprite_flags::center))
	{
		double width = begin - x;
		for (auto i = last_idx; i < sprite_queue.size(); i++)
		{
			auto& s = sprite_queue[i];
			s.x -= (width * 0.5f);
		}
	}
}


void xs::render::reload()
{
	sprite_queue.clear();
	sprites.clear();
}

int xs::render::load_image(const std::string& image_file)
{	
	// Find image first
	auto id = std::hash<std::string>{}(image_file);
	for (int i = 0; i < images.size(); i++)
		if (images[i].string_id == id)
			return i;

	auto buffer = fileio::read_binary_file(image_file);	
	internal::image img;
	img.string_id = id;
	GLubyte* data = stbi_load_from_memory(
		reinterpret_cast<unsigned char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		&img.width,
		&img.height,
		&img.channels,
		4);

	if (data == nullptr)
	{
		log::error("Image {} could not be loaded!", image_file);
		return -1;
	}
	
	create_texture_with_data(img, data);
	stbi_image_free(data);

	const auto i = images.size();
	images.push_back(img);
	return static_cast<int>(i);
}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	assert(image_id >= 0 && image_id < images.size());

	for(int i = 0; i < sprites.size(); i++)
	{
		const auto& s = sprites[i];
		if (s.image_id == image_id &&
			s.from.x == x0 && s.from.y == y0 &&
			s.to.x == x1 && s.to.y == y1)
			return i;
	}

	const auto i = sprites.size();
	sprite s = { image_id, { x0, y0 }, { x1, y1 } };
	sprites.push_back(s);
	return static_cast<int>(i);
}

void xs::render::render_sprite(
	int sprite_id,
	double x,
	double y,
	double z,
	double scale,
	double rotation,
	color mutiply,
	color add,
	unsigned int flags)
{
	assert(sprite_id >= 0);
	assert(sprite_id < internal::sprites.size());

	if (!tools::check_bit_flag_overlap(flags, sprite_flags::overlay)) {
		x += offset.x;
		y += offset.y;
	}

	sprite_queue.push_back({
		sprite_id,
		x,
		y,
		z,
		scale,
		rotation,
		mutiply,
		add,
		flags
	});
}

void xs::render::set_offset(double x, double y)
{
	xs::render::internal::offset = vec2((float)x, (float)y);
}

