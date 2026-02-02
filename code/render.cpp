#include "render.hpp"
#include "render_internal.hpp"
#include <ios>
#include <unordered_map>
#include <sstream>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define NANOSVG_IMPLEMENTATION	// Expand implementation
#include <nanosvg/nanosvg.h>

#include "configuration.hpp"
#include "fileio.hpp"
#include "log.hpp"
#include "tools.hpp"
#include "device.hpp"
#include "profiler.hpp"
#include "inspector.hpp"

// Include stb_image 
#ifdef PLATFORM_SWITCH
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma GCC diagnostic pop
#else
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#endif

// Include stb_truetype
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>
#ifdef PLATFORM_SWITCH
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include <stb/stb_easy_font.h>
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <stb/stb_easy_font.h>
#pragma GCC diagnostic pop
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

#if defined(PLATFORM_PC) || defined(PLATFORM_MAC)
#define CAN_RELOAD_IMAGES 1
#endif

using namespace glm;
using namespace std;

namespace xs::render
{
	std::vector<font_atlas>			fonts = {};
	std::vector<image>				images = {};
	glm::vec2						offset = glm::vec2(0.0f, 0.0f);

    int                             lines_count = 0;
    int                             lines_begin_count = 0;
    debug_vertex_format             lines_array[lines_max * 2];

    int                             triangles_count = 0;
    int                             triangles_begin_count = 0;
    debug_vertex_format             triangles_array[triangles_max * 3];

    dbg_primitive                   current_primitive = dbg_primitive::none;
    glm::vec4                       current_color = {1.0, 1.0, 1.0, 1.0};

	const int FONT_ATLAS_MIN_CHARACTER = 32;
	const int FONT_ATLAS_NR_CHARACTERS = 96;

#ifdef CAN_RELOAD_IMAGES
	std::vector<uint64_t> last_write_times;
#endif
}

using namespace xs;

int xs::render::load_font(const std::string& font_file, double size)
{	
	// Find image first
	auto id = std::hash<std::string>{}(font_file + std::to_string(size));
	for (size_t i = 0; i < fonts.size(); i++)
		if (fonts[i].string_id == id)
			return static_cast<int>(i);

	int font_id = (int)fonts.size();
	fonts.push_back(font_atlas());
	auto& font = fonts.back();

	font.buffer = fileio::read_binary_file(font_file);
	if (font.buffer.empty()) {
		log::error("Font file {} could not be loaded!", font_file);
		return -1;
	}
	font.buffer_ptr = reinterpret_cast<const unsigned char*>(font.buffer.data());

	// Get font info
	if (!stbtt_InitFont(&font.info, font.buffer_ptr, 0))
		log::info("initializing font has failed");
	
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
	img.width = dimension;
	img.height = dimension;
	auto bitmap = static_cast<unsigned char*>(malloc(img.width * img.height));
	assert(bitmap != nullptr);

	// pack font
	font.packed_chars.resize(FONT_ATLAS_NR_CHARACTERS);
	
	// pack font
	stbtt_pack_context pc;
	stbtt_PackBegin(&pc, bitmap, img.width, img.height, 0, 2, nullptr);
	stbtt_PackFontRange(
		&pc,
		font.buffer_ptr,
		0, (float)size,
		FONT_ATLAS_MIN_CHARACTER,
		FONT_ATLAS_NR_CHARACTERS,
		&font.packed_chars[0]);
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
	delete[] rgba;

	int image_id = (int)images.size();
	images.push_back(img);
	font.image_id = image_id;
	font.string_id = id;

#if DEBUG_FONT_ATLAS
	std::string filename = std::string("FontAtlas-") + std::to_string(size) + ".png";
	stbi_write_png(filename.c_str(), img.width, img.height, 4, rgba, 0);
#endif
	free(bitmap);

#if CAN_RELOAD_IMAGES
	last_write_times.push_back(0);
#endif

	return font_id;
}

int xs::render::get_image_height(int image_id)
{
	if (image_id < 0 || image_id >= static_cast<int>(images.size())) {
		log::error("get_image_height() image_id={} is invalid!", image_id);
		return -1;
	}
	auto& img = images[image_id];
	return img.height;
}

int xs::render::get_image_width(int image_id)
{
	if (image_id < 0 || image_id >= static_cast<int>(images.size())) {
		log::error("get_image_width() image_id={} is invalid!", image_id);
		return -1;
	}

	auto& img = images[image_id];
	return img.width;
}

void xs::render::shape(
	int sprite_id,
	double x, double y, double z,
	double size, double rotation, color mutiply, color add)
{
	xs::render::sprite(
		sprite_id, x, y, z,
		size, rotation, mutiply, add, is_shape);
}

void xs::render::text(
	int font_id,
	const std::string& text,
	double x,
	double y,
	double z,
	color multiply,
	color add,
	unsigned int flags)
{
	if(font_id < 0 || font_id >= static_cast<int>(fonts.size()))
	{
		log::error("render_text() font_id={} is invalid!", font_id);
		return;
	}

	auto& font = fonts[font_id];
	auto& img = images[font.image_id];

	double begin = x;

	// Do a first pass to calculate the width of the text
	if (tools::check_bit_flag_overlap(flags, xs::render::sprite_flags::center))
	{
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
			begin -= advance * 0.5;
		}
	}

	// Center text has been calculated, remove the flag
	flags = flags & ~xs::render::sprite_flags::center;

	// Render text
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
		xs::render::sprite(sprite, begin + bearing, y - quad.y1, z, 1, 0, multiply, add, flags);

		begin += advance + kerning;		
	}
}

int xs::render::load_image(const std::string& image_file)
{	
	// Find image first
	auto id = std::hash<std::string>{}(image_file);
	for (size_t i = 0; i < images.size(); i++)
		if (images[i].string_id == id)
			return static_cast<int>(i);

	auto buffer = fileio::read_binary_file(image_file);	
	image img;
	img.string_id = id;
	img.file = image_file;
	uchar* data = stbi_load_from_memory(
		reinterpret_cast<unsigned char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		&img.width,
		&img.height,
		&img.channels,
		4);

	if (data == nullptr)
	{
		log::error("Image {} could not be loaded!", image_file);
		xs::inspector::notify(
			xs::inspector::notification_type::error,
			"Image " + image_file +  " could not be loaded!",
			5);
		return -1;
	}
	
	create_texture_with_data(img, data);
	stbi_image_free(data);

	const auto i = images.size();
	images.push_back(img);	


#ifdef CAN_RELOAD_IMAGES
	auto tm = xs::fileio::last_write(image_file);
	last_write_times.push_back(tm);
#endif

	return static_cast<int>(i);
}

struct shape
{
	int image_id = -1;
	vector<float> positions;
	vector<float> texture_coordinates;
	unsigned int vertex_count;
	vector<unsigned short> indices;
	unsigned int index_count;
};

int render::load_shape(const std::string& shape_file)
{
	auto buffer = fileio::read_binary_file(shape_file);

	vector<vec2> positions;
	vector<vec2> texture_coordinates;
	vector<unsigned short> indices;
	
	NSVGimage* image = nsvgParse(reinterpret_cast<char*>(buffer.data()), "mm", 96);
	if (!image) {
		log::error("Failed to parse SVG file: {}", shape_file);
		return -1;
	}

	xs::log::info("SVG size: {} x {}", image->width, image->height);

	// Calculate bounds for the entire shape
	float min_x = FLT_MAX, min_y = FLT_MAX;
	float max_x = -FLT_MAX, max_y = -FLT_MAX;
	
	// First pass: collect all path points and calculate bounds
	vector<vec2> path_points;
	
	for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
		for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
			// Add path points (simplified - just using control points for now)
			for (int i = 0; i < path->npts; i++) {
				float x = path->pts[i * 2];
				float y = path->pts[i * 2 + 1];
				
				path_points.push_back(vec2(x, y));
				
				// Update bounds
				min_x = std::min(min_x, x);
				max_x = std::max(max_x, x);
				min_y = std::min(min_y, y);
				max_y = std::max(max_y, y);
			}
		}
	}
	
	// Calculate center point for fan mesh
	vec2 center = vec2((min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f);
	
	// Normalize coordinates to [0, 1] range for texture coordinates
	float width = max_x - min_x;
	float height = max_y - min_y;
	
	if (width == 0 || height == 0) {
		log::error("Invalid SVG dimensions");
		nsvgDelete(image);
		return -1;
	}
	
	// Create fan mesh
	// Add center vertex
	positions.push_back(center);
	texture_coordinates.push_back(vec2(0.5f, 0.5f)); // Center of texture
	
	// Add path points as vertices
	for (const auto& point : path_points) {
		positions.push_back(point);
		// Calculate texture coordinates (normalized)
		float u = (point.x - min_x) / width;
		float v = (point.y - min_y) / height;
		texture_coordinates.push_back(vec2(u, v));
	}
	
	// Create triangles for fan mesh
	// Each triangle connects center (index 0) with consecutive path points
	for (unsigned short i = 1; i < path_points.size(); i++) {
		 unsigned short next_i = (i % (path_points.size() - 1)) + 1;
		
		// Triangle: center -> current point -> next point
		indices.push_back(0);        // center
		indices.push_back(i);        // current point
		indices.push_back(next_i);   // next point
	}
	
	// Close the shape by connecting last point back to first
	if (path_points.size() > 2) {
		indices.push_back(0);        // center
		indices.push_back(static_cast<unsigned short>(path_points.size())); // last point
		indices.push_back(1);        // first point
	}
	
	nsvgDelete(image);
	
	// Create the shape using existing infrastructure
	// Use the first image as placeholder (you might want to create a default texture)
	int image_id = 0;
	//if (!images.empty()) {
	//	image_id = (int)images[0].string_id;
	//}
	
	auto shape_id = render::create_shape(
		image_id,
		&positions[0].x,
		&texture_coordinates[0].x,
		static_cast<unsigned int>(positions.size()),
		indices.data(),
		static_cast<unsigned int>(indices.size())
	);
	
	return shape_id;
}

void xs::render::set_offset(double x, double y)
{
	offset = vec2((float)x, (float)y);
}

void xs::render::dgb_begin(dbg_primitive p)
{
    if (current_primitive == dbg_primitive::none)
    {
        current_primitive = p;
        triangles_begin_count = 0;
        lines_begin_count = 0;
    }
    else
    {
        xs::log::error("Renderer begin()/end() mismatch! Primitive already active in begin().");
    }
}

void xs::render::dbg_vertex(double x, double y)
{
	x += offset.x;
	y += offset.y;
	
    if (current_primitive == dbg_primitive::triangles && triangles_count < triangles_max - 1)
    {
        const uint idx = triangles_count * 3;
        triangles_array[idx + triangles_begin_count].position = { x, y, 0.0f, 1.0f };
        triangles_array[idx + triangles_begin_count].color = current_color;
        triangles_begin_count++;
        if (triangles_begin_count == 3)
        {
            triangles_begin_count = 0;
            triangles_count++;
        }
    }
    else if (current_primitive == dbg_primitive::lines && lines_count < lines_max)
    {
        if (lines_begin_count == 0)
        {
            lines_array[lines_count * 2].position = { x, y, 0.0f, 1.0f };
            lines_array[lines_count * 2].color = current_color;
            lines_begin_count++;
        }
        else if(lines_begin_count == 1)
        {
            lines_array[lines_count * 2 + 1].position = { x, y, 0.0f, 1.0f };
            lines_array[lines_count * 2 + 1].color = current_color;
            lines_begin_count++;
            lines_count++;
        }
        else
        {
            // assert(lines_begin_count > 1 && lines_count > 1);
            lines_array[lines_count * 2].position = lines_array[lines_count * 2 - 1].position;
            lines_array[lines_count * 2].color = lines_array[lines_count * 2 - 1].color;
            lines_array[lines_count * 2 + 1].position = { x, y, 0.0f, 1.0f };
            lines_array[lines_count * 2 + 1].color = current_color;
            lines_begin_count++;
            lines_count++;
        }
    }
}

void xs::render::dbg_end()
{
    if(current_primitive == dbg_primitive::none)
    {
        log::error("Renderer begin()/end() mismatch! No primitive active in end().");
        return;
    }
    
    current_primitive = dbg_primitive::none;
    if (triangles_begin_count != 0 /* TODO: lines */)
    {
        log::error("Renderer vertex()/end() mismatch!");
    }

}

void xs::render::dbg_color(color c)
{
    current_color.r = c.r / 255.0f;
    current_color.g = c.g / 255.0f;
    current_color.b = c.b / 255.0f;
    current_color.a = c.a / 255.0f;
}

void xs::render::dbg_line(double x0, double y0, double x1, double y1)
{
	x0 += offset.x;
	y0 += offset.y;
	x1 += offset.x;
	y1 += offset.y;
    if (lines_count < lines_max)
    {
        lines_array[lines_count * 2].position = {x0, y0, 0.0f, 1.0f};
        lines_array[lines_count * 2 + 1].position = {x1, y1, 0.0f, 1.0f};
        lines_array[lines_count * 2].color = current_color;
        lines_array[lines_count * 2 + 1].color = current_color;
        ++lines_count;
    }

}

void xs::render::dbg_text(const std::string& text, double x, double y, double size)
{
	x += offset.x;
	y += offset.y;
    struct stbVec
    {
        float x;
        float y;
        float z;
        unsigned char color[4];
    };

    static stbVec vertexBuffer[2048];

    const auto n = text.length();
    char* asChar = new char[n + 1];
    strcpy(asChar, text.c_str());
    const int numQuads = stb_easy_font_print(0, 0, asChar, nullptr, vertexBuffer, sizeof(vertexBuffer));
    delete[] asChar;

    const vec4 origin(x, y, 0.0f, 0.0f);
    const auto s = static_cast<float>(size);
    for (int i = 0; i < numQuads; i++)
    {
        const auto& v0 = vertexBuffer[i * 4 + 0];
        const auto& v1 = vertexBuffer[i * 4 + 1];
        const auto& v2 = vertexBuffer[i * 4 + 2];
        const auto& v3 = vertexBuffer[i * 4 + 3];

        const uint idx = triangles_count * 3;
        triangles_array[idx + 0].position = s * vec4(v0.x, -v0.y, v0.z, 1.0f) + origin;
        triangles_array[idx + 2].position = s * vec4(v1.x, -v1.y, v1.z, 1.0f) + origin;
        triangles_array[idx + 1].position = s * vec4(v2.x, -v2.y, v2.z, 1.0f) + origin;
        triangles_array[idx + 3].position = s * vec4(v2.x, -v2.y, v2.z, 1.0f) + origin;
        triangles_array[idx + 4].position = s * vec4(v3.x, -v3.y, v3.z, 1.0f) + origin;
        triangles_array[idx + 5].position = s * vec4(v0.x, -v0.y, v0.z, 1.0f) + origin;

        triangles_array[idx + 0].color = current_color;
        triangles_array[idx + 1].color = current_color;
        triangles_array[idx + 2].color = current_color;

        triangles_array[idx + 3].color = current_color;
        triangles_array[idx + 4].color = current_color;
        triangles_array[idx + 5].color = current_color;

        triangles_count += 2;

        if (triangles_count >= triangles_max)
            return;
    }
}

#ifdef CAN_RELOAD_IMAGES

int xs::render::reload_images()
{
	int reloaded = 0;
	for (size_t i = 0; i < images.size(); i++) {
		auto& image = images[i];

		if (image.file.empty())
			continue;

		auto last_time = last_write_times[i];
		
		auto new_time = xs::fileio::last_write(image.file);

		if (new_time > last_time) {
			auto buffer = fileio::read_binary_file(image.file);
			uchar* data = stbi_load_from_memory(
				reinterpret_cast<unsigned char*>(buffer.data()),
				static_cast<int>(buffer.size()),
				&image.width,
				&image.height,
				&image.channels,
				4);

			if (data != nullptr)
			{
				log::info("Image {} reloaded!", image.file);
				auto message = XS_FORMAT("Image {} reloaded!", image.file);
				inspector::notify(inspector::notification_type::success, message, 4.0f);
				last_write_times[i] = new_time;
				create_texture_with_data(image, data);
				stbi_image_free(data);
				reloaded++;	
			}
			else
			{
				log::error("Image {} could not be reloaded!", image.file);
				auto message = XS_FORMAT("Image {} could not be reloaded!", image.file);
				inspector::notify(inspector::notification_type::error, message, 5.0f);
			}
		}
	}
	return reloaded;
}

#else 

int xs::render::reload_images() { return 0; }

#endif


