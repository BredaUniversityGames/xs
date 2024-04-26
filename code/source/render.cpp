#include "render.h"
#include "render_internal.h"
#include <ios>
#include <sstream>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "configuration.h"
#include "fileio.h"
#include "log.h"
#include "tools.h"
#include "device.h"
#include "profiler.h"

// Include stb_image 
#ifdef PLATFORM_SWITCH
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#pragma clang diagnostic pop
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

using namespace std;
using namespace glm;

namespace xs::render::internal
{
	std::vector<sprite_queue_entry>	sprite_queue = {};
	std::vector<sprite>				sprites = {};
	std::vector<font_atlas>			fonts = {};
	std::vector<image>				images = {};
	glm::vec2						offset = glm::vec2(0.0f, 0.0f);

    int                             lines_count = 0;
    int                             lines_begin_count = 0;
    debug_vertex_format             lines_array[lines_max * 2];

    int                             triangles_count = 0;
    int                             triangles_begin_count = 0;
    debug_vertex_format             triangles_array[triangles_max * 3];

    primitive                       current_primitive = primitive::none;
    glm::vec4                       current_color = {1.0, 1.0, 1.0, 1.0};

	std::vector<directional_light>	dir_lights;
	std::vector<point_light>		point_lights;

	const int FONT_ATLAS_MIN_CHARACTER = 32;
	const int FONT_ATLAS_NR_CHARACTERS = 96;

#ifdef CAN_RELOAD_IMAGES
	std::vector<uint64_t> last_write_times;
#endif
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
	//int val = ascent - descent;
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

int xs::render::load_mesh(const std::string& mesh_file)
{
	// auto id = std::hash<std::string>{}(mesh_file); TODO: Implement

	if(fileio::exists(mesh_file) == false)
	{
		log::error("Mesh file {} does not exist!", mesh_file);
		return -1;
	}

	auto data = fileio::read_text_file(mesh_file);
	if (data.empty())
	{
		log::error("Mesh file {} could not be loaded!", mesh_file);
		return -1;
	}

	std::stringstream ss(data);

	char input[512];
	char line[512];

	float x, y, z, u, v;
	unsigned k, l, m;

	vector<vec3>        positions;
	vector<vec2>        texcoords;
	vector<vec3>        normals;
	vector<vec3>        colors;
	vector<unsigned>    indices;

	vector<vec3>        final_positions;
	vector<vec2>        final_texcoords;
	vector<vec3>        final_normals;
	vector<vec3>        final_colors;
	vector<unsigned>    final_indices;

	unsigned idx = 0;
	map<unsigned, unsigned> index_for_vertex;


	string object_name = "";
	bool search = !object_name.empty();
	bool found = !search;
	
	int first_position = 0;
	int first_texcoord = 0;
	int first_normal = 0;

	while (ss >> input)
	{
		if (search && input[0] == 'o')
		{
			if (!found)
			{
				string name;
				ss >> name;
				found = (name == object_name);
			}
			else
			{
				break;
			}
		}

		if (input[0] == '#')
		{
			// Comment, skip line
			ss.getline(line, 255, '\n');
		}
		else if (strcmp(input, "v") == 0)
		{
			// Position
			ss >> x >> y >> z;
			if (found)
				positions.push_back(vec3(x, y, z));
			else
				first_position++;

		}
		else if (strcmp(input, "vt") == 0)
		{
			// Texture coord
			ss >> u >> v;
			if (found)
				texcoords.push_back(vec2(u, v));
			else
				first_texcoord++;

		}
		else if (strcmp(input, "vn") == 0)
		{
			// Vertex normal
			ss >> x >> y >> z;
			if (found)
				normals.push_back(vec3(x, y, z));
			else
				first_normal++;

		}
		else if (strcmp(input, "vc") == 0)
		{
			// Vertex color
			ss >> x >> y >> z;
			colors.push_back(vec3(x, y, z));
		}
		else if (strcmp(input, "f") == 0)
		{
			if (!found)
				continue;

			char slash;
			// Three vertices on a triangle
			for (int i = 0; i < 3; ++i)
			{
				// Read indices
				ss >> k >> slash >> l >> slash >> m;
				k -= 1;
				l -= 1;
				m -= 1;

				final_positions.push_back(positions[k - first_position]);
				final_texcoords.push_back(texcoords[l - first_texcoord]);
				final_normals.push_back(normals[m - first_normal]);
				final_colors.push_back(vec3(1, 1, 1)); // TODO: add support for colors
				final_indices.push_back(idx);
				idx++;


				/*
				// Read indices
				ss >> k >> slash >> l >> slash >> m;
				k -= 1;
				l -= 1;
				m -= 1;

				// Generate unique hash
				auto h = tri_hash(k, l, m);

				// Check if this a new vertex
				auto itr = index_for_vertex.find(h);
				// indexForVertex.end();//
				if ((false) && itr != index_for_vertex.end())
				{
					// Vertex in set
					indicesVec.push_back(itr->second);
				}
				else
				{
					// New vertex
					VertexPositionNormalTexture vertex;
					vertex.Position = offsetVec[k];
					vertex.Normal = normalVec[m];
					vertex.Texture = textureVec[l];

					// Add it to the vector
					vertexVec.push_back(vertex);
					// Book-keep index by vertex hash
					indexForVertex[hash] = idx;
					// Add index to indices vector
					indicesVec.push_back(idx);
					idx++;
				}
				*/
			}
		}
	}

	// Create mesh
	return create_mesh(
		final_indices.data(),
		(unsigned)final_indices.size(),
		value_ptr(final_positions[0]),
		value_ptr(final_normals[0]),
		value_ptr(final_texcoords[0]),
		value_ptr(final_colors[0]),
		(unsigned)final_positions.size());
}

int xs::render::get_image_height(int image_id)
{
	if (image_id < 0 || image_id >= images.size()) {
		log::error("get_image_height() image_id={} is invalid!", image_id);
		return -1;
	}
	auto& img = images[image_id];
	return img.height;
}

int xs::render::get_image_width(int image_id)
{
	if (image_id < 0 || image_id >= images.size()) {
		log::error("get_image_width() image_id={} is invalid!", image_id);
		return -1;
	}

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

void xs::render::directional_light(
	const glm::vec4& direction,
	const glm::vec4& position,
	const glm::vec4& shadow_volume,
	const glm::vec4& color_and_intensity)
{
	if (internal::dir_lights.size() >= 4)
	{
		log::warn("xs supports up to 4 directional lights!");
		return;
	}		

	// Add to the list of active directional lights	
	internal::dir_lights.push_back({
		direction,
		position,
		shadow_volume,		
		vec3(color_and_intensity)
	});
}

void xs::render::render_point_light(
	const float* position,
	float radius,
	const float* color_and_intensity)
{
	if (internal::point_lights.size() >= 4)
		return;	// TODO : Log error

	// Add to the list of active point lights	
	point_lights.push_back({
		{position[0], position[1], position[2]},
		radius,
		{color_and_intensity[0],color_and_intensity[1], color_and_intensity[2]}
	});
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

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	if (image_id < 0 || image_id >= images.size()) {
		log::error("Can't create sprite with image {}!", image_id);
		return -1;
	}

	const auto epsilon = 0.001;
	for(int i = 0; i < sprites.size(); i++)
	{
		const auto& s = sprites[i];
		if (s.image_id == image_id &&
			abs(s.from.x - x0) < epsilon &&
			abs(s.from.y - y0) < epsilon &&
			abs(s.to.x - x1) < epsilon &&
			abs(s.to.y - y1) < epsilon)
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
	color multiply,
	color add,
	unsigned int flags)
{
	if (sprite_id < 0 || sprite_id >= internal::sprites.size()) {
		log::error("Can't render sprite {}!", sprite_id);
		return;
	}

	if (!tools::check_bit_flag_overlap(flags, sprite_flags::overlay)) {
		x += offset.x;
		y += offset.y;
	}
    
    //x = round(x);
    //y = round(y);

	sprite_queue.push_back({
		sprite_id,
		x,
		y,
		z,
		scale,
		rotation,
		multiply,
		add,
		flags
	});
}

void xs::render::set_offset(double x, double y)
{
	internal::offset = vec2((float)x, (float)y);
}

void xs::render::set_3d_projection(const glm::mat4& projection)
{
	internal::projection = projection;
}

void xs::render::set_3d_view(const glm::mat4& view)
{
	internal::view = view;
}

void xs::render::begin(primitive p)
{
    if (current_primitive == primitive::none)
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

void xs::render::vertex(double x, double y)
{
    if (current_primitive == primitive::triangles && triangles_count < triangles_max - 1)
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
    else if (current_primitive == primitive::lines && lines_count < lines_max)
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

void xs::render::end()
{
    if(current_primitive == primitive::none)
    {
        log::error("Renderer begin()/end() mismatch! No primitive active in end().");
        return;
    }
    
    current_primitive = primitive::none;
    if (triangles_begin_count != 0 /* TODO: lines */)
    {
        log::error("Renderer vertex()/end() mismatch!");
    }

}

void xs::render::set_color(color c)
{
    current_color.r = c.r / 255.0f;
    current_color.g = c.g / 255.0f;
    current_color.b = c.b / 255.0f;
    current_color.a = c.a / 255.0f;
}

void xs::render::line(double x0, double y0, double x1, double y1)
{
    if (lines_count < lines_max)
    {
        lines_array[lines_count * 2].position = {x0, y0, 0.0f, 1.0f};
        lines_array[lines_count * 2 + 1].position = {x1, y1, 0.0f, 1.0f};
        lines_array[lines_count * 2].color = current_color;
        lines_array[lines_count * 2 + 1].color = current_color;
        ++lines_count;
    }

}

void xs::render::text(const std::string& text, double x, double y, double size)
{
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

void xs::render::reload_images()
{
	for (int i = 0; i < images.size(); i++) {
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

			if (data == nullptr)
			{
				log::error("Image {} could not be reloaded!", image.file);
				return;
			}
			else
			{
				log::info("Image {} reloaded!", image.file);
			}
			last_write_times[i] = new_time;
			create_texture_with_data(image, data);
			stbi_image_free(data);
		}
	}
}

#else 

void xs::render::reload_images() {}

#endif


