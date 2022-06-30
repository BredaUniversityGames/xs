#include "render.h"
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
#include "opengl.h"
#include "configuration.h"
#include "fileio.h"
#include "log.h"
#include "tools.h"
#include "device.h"
#include "profiler.h"

#define DEBUG_FONT_ATLAS 0

#if DEBUG_FONT_ATLAS
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include <stb/stb_image_write.h>
#endif

using namespace glm;

namespace xs::render::internal
{
	struct image
	{
		GLuint		gl_id		= 0;
		int			width		= -1;
		int			height		= -1;
		int			channels	= -1;
		std::size_t	string_id	= 0;
	};

	struct font_atals
	{
		int image_id								= -1;
		std::size_t	string_id						= 0;
		std::vector<stbtt_packedchar> packed_chars	= {};
		stbtt_fontinfo info							= {};
		unsigned char* buffer						= nullptr;
		double scale								= 0;
	};


	
	void create_frame_buffers();
	//TODO: void delete_frame_buffers();
	void compile_draw_shader();
	void compile_sprite_shader();
	bool compile_shader(GLuint* shader, GLenum type, const GLchar* source);
	bool link_program(GLuint program);
	void create_gl_texture_with_data(image& img, uchar* data);
	vec4 to_vec4(color c) { return vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f); }

	int width = -1;
	int height = -1;
	vec2 offset = vec2(0.0f, 0.0f);

	unsigned int render_fbo;
	unsigned int render_texture;
	
	vec4					current_color;
	struct vertex_format { vec3 position; vec4 color; };

	unsigned int			shader_program = 0;
	int const				lines_max = 16000;
	int						lines_count = 0;
	int						lines_begin_count = 0;
	vertex_format			vertex_array[lines_max * 2];
	unsigned int			lines_vao = 0;
	unsigned int			lines_vbo = 0;

	int const				triangles_max = 21800;
	int						triangles_count = 0;
	int						triangles_begin_count = 0;
	vertex_format			triangles_array[triangles_max * 3];
	unsigned int			triangles_vao = 0;
	unsigned int			triangles_vbo = 0;

	primitive				current_primitive = primitive::none;

	struct sprite_vtx_format
	{
		vec3 position;
		vec2 texture;
		vec4 add_color;
		vec4 mul_color;
	};	
	struct sprite { int image_id; vec2 from; vec2 to; };
	struct sprite_queue_entry
	{
		int sprite_id			=	-1;
		double x				= 0.0;
		double y				= 0.0;
		double scale			= 1.0;
		double rotation			= 0.0;
		// sprite_anchor anchor	= {};	// TODO: Remove	
		color mul_color			= {};
		color add_color			= {};
		unsigned int flags		= 0;
	};
	
	unsigned int			sprite_program = 0;
	int const				sprite_trigs_max = 21800;
	int						sprite_trigs_count = 0;
	sprite_vtx_format		sprite_trigs_array[sprite_trigs_max * 3];
	unsigned int			sprite_trigs_vao = 0;
	unsigned int			sprite_trigs_vbo = 0;
	std::vector<sprite_queue_entry> sprite_queue = {};
	std::vector<image>		images = {};
	std::vector<sprite>		sprites = {};
	std::vector<font_atals> fonts = {};

	//color white = { 255, 255, 255, 255 };
	//color black = { 255, 0, 0, 0 };

	void rotate_vector3d(vec3& vec, float radians);
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
	fonts.push_back(font_atals());
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

	image img;
	img.width = 256;
	img.height = 256;
	auto bitmap = static_cast<unsigned char*>(malloc(img.width * img.height));	

	stbtt_pack_context pc;	
	stbtt_PackBegin(&pc, bitmap, img.width, img.height, 0, 2, nullptr);
	
	int ascent;
	int descent;
	int lineGap;
	stbtt_GetFontVMetrics(&font.info, &ascent, &descent, &lineGap);
	font.scale = (size / double(ascent - descent));

	// pack font
	font.packed_chars.resize(96);

	stbtt_PackFontRange(&pc, font.buffer, 0, (float)size, 33, 92, &font.packed_chars[0]);

	// Apply
	stbtt_PackEnd(&pc);

	auto n = img.width * img.height;
	auto rgba = new color[n];
	for (int i = 0; i < n; i++) {
		color c;
		c.rgba[0] = 255;	// Channels are flipped here :D
		c.rgba[1] = 255;
		c.rgba[2] = 255;
		c.rgba[3] = bitmap[i] > 196 ? 255 : 0;
		rgba[i] = c;
	}


	img.channels = 4;
	img.string_id = id;
	create_gl_texture_with_data(img, (uchar*)rgba);

	int image_id = (int)images.size();
	images.push_back(img);
	font.image_id = image_id;
	font.string_id = id;

#if DEBUG_FONT_ATLAS
	stbi_write_png("FontAtlas.png", img.width, img.height, 4, rgba, 0);
#endif
	free(bitmap);

	return font_id;
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
		const int charIndex = text[i] - 33;

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

		// glyph.Advance = static_cast<float>(advance);
		// glyph.BearingX = static_cast<float>(bearing);

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
		render_sprite(sprite, begin + bearing, y - quad.y1, 0, 1, multiply, add, 0);

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


void xs::render::initialize()
{
	width = configuration::width;
	height = configuration::height;

	internal::create_frame_buffers();
	internal::compile_draw_shader();
	internal::compile_sprite_shader();

	///////// Trigs //////////////////////
	glCreateVertexArrays(1, &lines_vao);
	glBindVertexArray(lines_vao);

	// Allocate VBO
	glGenBuffers(1, &lines_vbo);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);

	// Allocate into VBO
	const auto size = sizeof(vertex_array);
	glBufferData(GL_ARRAY_BUFFER, size, &vertex_array[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_format),
		reinterpret_cast<void*>(offsetof(vertex_format, position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_format),
		reinterpret_cast<void*>(offsetof(vertex_format, color)));

	XS_DEBUG_ONLY(glBindVertexArray(0));


	///////// Trigs //////////////////////
	glGenVertexArrays(1, &triangles_vao);
	glBindVertexArray(triangles_vao);

	// Allocate VBO
	glGenBuffers(1, &triangles_vbo);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, triangles_vbo);

	// Allocate into VBO
	const auto trigs_size = sizeof(triangles_array);
	glBufferData(GL_ARRAY_BUFFER, trigs_size, &triangles_array[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_format),
		reinterpret_cast<void*>(offsetof(vertex_format, position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_format),
		reinterpret_cast<void*>(offsetof(vertex_format, color)));

	XS_DEBUG_ONLY(glBindVertexArray(0));


	///////// Sprite //////////////////////
	glGenVertexArrays(1, &sprite_trigs_vao);
	glBindVertexArray(sprite_trigs_vao);

	// Allocate VBO
	glGenBuffers(1, &sprite_trigs_vbo);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, sprite_trigs_vbo);

	// Allocate into VBO
	const auto sprite_trigs_size = sizeof(sprite_trigs_array);
	glBufferData(GL_ARRAY_BUFFER, sprite_trigs_size, &sprite_trigs_array[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE, sizeof(sprite_vtx_format),
		reinterpret_cast<void*>(offsetof(sprite_vtx_format, position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 2, GL_FLOAT, GL_FALSE, sizeof(sprite_vtx_format),
		reinterpret_cast<void*>(offsetof(sprite_vtx_format, texture)));


	glEnableVertexAttribArray(3);
	glVertexAttribPointer(
		3, 4, GL_FLOAT, GL_FALSE, sizeof(sprite_vtx_format),
		reinterpret_cast<void*>(offsetof(sprite_vtx_format, mul_color)));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(
		4, 4, GL_FLOAT, GL_FALSE, sizeof(sprite_vtx_format),
		reinterpret_cast<void*>(offsetof(sprite_vtx_format, add_color)));

	XS_DEBUG_ONLY(glBindVertexArray(0));
}

void xs::render::shutdown()
{
	glDeleteProgram(shader_program);
	glDeleteVertexArrays(1, &lines_vao);
	glDeleteBuffers(1, &lines_vbo);

	// TODO: Delete all images
	// TODO: Delete frame buffer
}

void xs::render::set_offset(double x, double y)
{
	xs::render::internal::offset = vec2((float)x, (float)y);
}

void xs::render::render()
{	
	XS_PROFILE_SECTION("xs::render::render");
	auto w = width / 2.0f;
	auto h = height / 2.0f;
	mat4 p = ortho(-w, w, -h, h, -100.0f, 100.0f);
	mat4 v = lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 vp = p * v;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(sprite_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));

	for (const auto& spe : sprite_queue)
	{
		const auto& sprite = sprites[spe.sprite_id];
		const auto& image = images[sprite.image_id];

		auto from_x = 0.0; // spe.x;
		auto from_y = 0.0; // spe.y;
		auto to_x = /* spe.x + */ image.width * (sprite.to.x - sprite.from.x) * spe.scale;
		auto to_y = /* spe.y + */ image.height * (sprite.to.y - sprite.from.y) * spe.scale;

		auto from_u = sprite.from.x;
		auto from_v = sprite.from.y;
		auto to_u = sprite.to.x;
		auto to_v = sprite.to.y;

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_x))
			std::swap(from_u, to_u);

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::flip_y))
			std::swap(from_v, to_v);

		vec4 add_color = to_vec4(spe.add_color);
		vec4 mul_color = to_vec4(spe.mul_color);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image.gl_id);

		//if (triangles_count < triangles_max - 1)
		{
			sprite_trigs_array[0].position = { from_x, from_y, 0.0 };
			sprite_trigs_array[1].position = { from_x, to_y, 0.0 };
			sprite_trigs_array[2].position = { to_x, to_y, 0.0 };
			sprite_trigs_array[3].position = { to_x, to_y, 0.0 };
			sprite_trigs_array[4].position = { to_x, from_y, 0.0 };
			sprite_trigs_array[5].position = { from_x, from_y, 0.0 };

			sprite_trigs_array[0].texture = { from_u,	to_v };
			sprite_trigs_array[1].texture = { from_u,	from_v };
			sprite_trigs_array[2].texture = { to_u,		from_v };
			sprite_trigs_array[3].texture = { to_u,		from_v };
			sprite_trigs_array[4].texture = { to_u,		to_v };
			sprite_trigs_array[5].texture = { from_u,	to_v };

			for (int i = 0; i < 6; ++i)
			{
				sprite_trigs_array[i].add_color = add_color;
				sprite_trigs_array[i].mul_color = mul_color;				
			}
			
			vec3 anchor((to_x - from_x) * 0.5f, (to_y - from_y) * 0.5f, 0.0f);
			if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center))
			{				
				for (int i = 0; i < 6; i++)
					sprite_trigs_array[i].position -= anchor;
			}

			if (spe.rotation != 0.0)
			{
				for (int i = 0; i < 6; i++)
					rotate_vector3d(sprite_trigs_array[i].position, (float)spe.rotation);
			}

			for (int i = 0; i < 6; i++)
			{
				sprite_trigs_array[i].position.x += (float)spe.x;
				sprite_trigs_array[i].position.y += (float)spe.y;
			}

			
		}

		glBindVertexArray(sprite_trigs_vao);
		glBindBuffer(GL_ARRAY_BUFFER, sprite_trigs_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(sprite_vtx_format) * 6, &sprite_trigs_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	XS_DEBUG_ONLY(glUseProgram(0));
	XS_DEBUG_ONLY(glBindVertexArray(0));
	
	glUseProgram(shader_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));

	if (triangles_count > 0)
	{
		glBindVertexArray(triangles_vao);
		glBindBuffer(GL_ARRAY_BUFFER, triangles_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_format) * triangles_count * 3, &triangles_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, triangles_count * 3);
		XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}

	if (lines_count > 0)
	{
		glBindVertexArray(lines_vao);
		glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_format) * lines_count * 2, &vertex_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, lines_count * 2);
		XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}


	XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	XS_DEBUG_ONLY(glUseProgram(0));
	XS_DEBUG_ONLY(glBindVertexArray(0));

	// Blit render result screen
	//const auto mul = configuration::multiplier;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, render_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, xs::device::get_width(), xs::device::get_height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void xs::render::clear()
{
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glViewport(0, 0, width, height);
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	lines_count = 0;
	triangles_count = 0;
	sprite_queue.clear();
}

void xs::render::internal::create_gl_texture_with_data(xs::render::internal::image& img, uchar* data)
{
	GLint format = GL_INVALID_VALUE;
	GLint usage = GL_INVALID_VALUE;
	switch (img.channels)
	{
	case 1:
		format = GL_R8;
		usage = GL_RED;
		break;
	case 4:
		format = GL_RGBA;
		usage = GL_RGBA;
		break;
	case 3:
		format = GL_RGB;
		usage = GL_RGBA;
		break;
	default:
		assert(false);
	}

	glGenTextures(1, &img.gl_id);
	glBindTexture(GL_TEXTURE_2D, img.gl_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(
		GL_TEXTURE_2D,						// What (target)
		0,									// Mip-map level
		format,								// Internal format
		img.width,							// Width
		img.height,							// Height
		0,									// Border
		usage,								// Format (how to use)
		GL_UNSIGNED_BYTE,					// Type   (how to interpret)
		data);								// Data
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
	
	create_gl_texture_with_data(img, data);
	stbi_image_free(data);

	const auto i = images.size();
	images.push_back(img);
	return static_cast<int>(i);
}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
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
	double scale,
	double rotation,
	color mutiply,
	color add,
	unsigned int flags)
{
	// TODO: Validate sprite
	// int sprite_id = -1;
	// double x = 0.0;
	// double y = 0.0;
	// sprite_anchor anchor = {};
	// unsigned int flags = 0;
	// color mul_color = {};
	// color add_color = {};

	sprite_queue.push_back({
		sprite_id,
		x + offset.x,
		y + offset.y,
		scale,
		rotation,
		mutiply,
		add,
		flags
	});
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
		log::error("Renderer begin()/end() mismatch! Primitive already active in begin().");
	}
}

void xs::render::vertex(double x, double y)
{
	if (current_primitive == primitive::triangles && triangles_count < triangles_max - 1)
	{
		const uint idx = triangles_count * 3;
		triangles_array[idx + triangles_begin_count].position = { x, y, 0.0f };
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
			vertex_array[lines_count * 2].position = { x, y, 0.0f };
			vertex_array[lines_count * 2].color = current_color;
			lines_begin_count++;
		}
		else if(lines_begin_count == 1)
		{
			vertex_array[lines_count * 2 + 1].position = { x, y, 0.0f };
			vertex_array[lines_count * 2 + 1].color = current_color;
			lines_begin_count++;
			lines_count++;
		}
		else 
		{
			// assert(lines_begin_count > 1 && lines_count > 1);
			vertex_array[lines_count * 2].position = vertex_array[lines_count * 2 - 1].position;
			vertex_array[lines_count * 2].color = vertex_array[lines_count * 2 - 1].color;
			vertex_array[lines_count * 2 + 1].position = { x, y, 0.0f };
			vertex_array[lines_count * 2 + 1].color = current_color;
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

void xs::render::set_color(double r, double g, double b, double a)
{
	current_color.r = static_cast<float>(r);
	current_color.g = static_cast<float>(g);
	current_color.b = static_cast<float>(b);
	current_color.a = static_cast<float>(a);
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
		vertex_array[lines_count * 2].position = {x0, y0, 0.0f};
		vertex_array[lines_count * 2 + 1].position = {x1, y1, 0.0f};
		vertex_array[lines_count * 2].color = current_color;
		vertex_array[lines_count * 2 + 1].color = current_color;
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

	const vec3 origin(x, y, 0.0f);
	const auto s = static_cast<float>(size);
	for (int i = 0; i < numQuads; i++)
	{
		const auto& v0 = vertexBuffer[i * 4 + 0];
		const auto& v1 = vertexBuffer[i * 4 + 1];
		const auto& v2 = vertexBuffer[i * 4 + 2];
		const auto& v3 = vertexBuffer[i * 4 + 3];

		const uint idx = triangles_count * 3;
		triangles_array[idx + 0].position = s * vec3(v0.x, -v0.y, v0.z) + origin;
		triangles_array[idx + 2].position = s * vec3(v1.x, -v1.y, v1.z) + origin;
		triangles_array[idx + 1].position = s * vec3(v2.x, -v2.y, v2.z) + origin;
		triangles_array[idx + 3].position = s * vec3(v2.x, -v2.y, v2.z) + origin;
		triangles_array[idx + 4].position = s * vec3(v3.x, -v3.y, v3.z) + origin;
		triangles_array[idx + 5].position = s * vec3(v0.x, -v0.y, v0.z) + origin;

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

void xs::render::internal::create_frame_buffers()
{
	glCreateFramebuffers(1, &render_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glGenTextures(1, &render_texture);
	glBindTexture(GL_TEXTURE_2D, render_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture, 0);
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		assert(false);
	XS_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));

}

void xs::render::internal::compile_sprite_shader()
{
	auto vs_str = xs::fileio::read_text_file("[games]/shared/shaders/sprite.vert");
	auto fs_str = xs::fileio::read_text_file("[games]/shared/shaders/sprite.frag");
	const char* const vs_source = vs_str.c_str();
	const char* const fs_source = fs_str.c_str();

	GLuint vert_shader = 0;
	GLuint frag_shader = 0;

	sprite_program = glCreateProgram();

	GLboolean res = compile_shader(&vert_shader, GL_VERTEX_SHADER, vs_source);
	if (!res)
	{
		log::error("Renderer failed to compile sprite vertex shader");
		return;
	}

	res = compile_shader(&frag_shader, GL_FRAGMENT_SHADER, fs_source);
	if (!res)
	{
		log::error("Renderer failed to compile sprite fragment shader");
		return;
	}

	glAttachShader(sprite_program, vert_shader);
	glAttachShader(sprite_program, frag_shader);

	if (!link_program(sprite_program))
	{
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
		glDeleteProgram(sprite_program);
		log::error("Renderer failed to link sprite shader program");
		return;
	}

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
}

void xs::render::internal::compile_draw_shader()
{
	const auto* const vs_source =
		"#version 460 core												\n\
		layout (location = 1) in vec3 a_position;						\n\
		layout (location = 2) in vec4 a_color;							\n\
		layout (location = 1) uniform mat4 u_worldviewproj;				\n\
		out vec4 v_color;												\n\
																		\n\
		void main()														\n\
		{																\n\
			v_color = a_color;											\n\
			gl_Position = u_worldviewproj * vec4(a_position, 1.0);		\n\
		}";

	const auto* const fs_source =
		"#version 460 core												\n\
		in vec4 v_color;												\n\
		out vec4 frag_color;											\n\
																		\n\
		void main()														\n\
		{																\n\
			frag_color = v_color;										\n\
		}";

	GLuint vert_shader = 0;
	GLuint frag_shader = 0;

	shader_program = glCreateProgram();

	GLboolean res = compile_shader(&vert_shader, GL_VERTEX_SHADER, vs_source);
	if (!res)
	{
		log::error("Renderer failed to compile vertex shader");
		return;
	}

	res = compile_shader(&frag_shader, GL_FRAGMENT_SHADER, fs_source);
	if (!res)
	{
		log::error("Renderer failed to compile fragment shader");
		return;
	}

	glAttachShader(shader_program, vert_shader);
	glAttachShader(shader_program, frag_shader);

	if (!link_program(shader_program))
	{
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
		glDeleteProgram(shader_program);
		log::error("Renderer failed to link shader program");
		return;
	}

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

}

bool xs::render::internal::compile_shader(GLuint* shader, GLenum type, const GLchar* source)
{
	GLint status;

	if (!source)
	{
		xs::log::error("Failed to compile empty shader");
		return false;
	}

	*shader = glCreateShader(type);
	glShaderSource(*shader, 1, &source, nullptr);

	glCompileShader(*shader);

#if defined(DEBUG)
	GLint log_length = 0;
	glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 1)
	{
		GLchar* log = static_cast<GLchar*>(malloc(log_length));
		glGetShaderInfoLog(*shader, log_length, &log_length, log);
		if (log)
			xs::log::error("Program compile log: {}", log);
		free(log);
	}
#endif

	glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
	if (status == 0)
	{
		glDeleteShader(*shader);
		return false;
	}

	return true;
}

bool xs::render::internal::link_program(GLuint program)
{
	GLint status;

	glLinkProgram(program);

#if defined(DEBUG)
	GLint logLength = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 1)
	{
		GLchar* log = static_cast<GLchar*>(malloc(logLength));
		glGetProgramInfoLog(program, logLength, &logLength, log);
		if (log)
			xs::log::error("Program link log: {}", log);
		free(log);
	}
#endif

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	return status != 0;
}

void xs::render::internal::rotate_vector3d(vec3& vec, float radians)
{
	const float x = cos(radians) * vec.x - sin(radians) * vec.y;
	const float y = sin(radians) * vec.x + cos(radians) * vec.y;
	vec.x = x;
	vec.y = y;
}
