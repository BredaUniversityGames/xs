#include "render.h"
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_easy_font.h>
#include "opengl.h"
#include "configuration.h"
#include "fileio.h"

#include "log.h"

using namespace glm;

namespace xs::render::internal
{
	struct image
	{
		GLuint	gl_id		= 0;
		int		width		= -1;
		int		height		= -1;
		int		channels	= -1;
	};
	
	// TODO: Implement later
	// void create_frame_buffers();
	// void delete_frame_buffers();
	bool compile_shader(GLuint* shader, GLenum type, const GLchar* source);
	bool link_program(GLuint program);

	int width = -1;
	int height = -1;
	// Create G-buffer
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

	struct sprite_vtx_format { vec3 position; vec2 texture; vec4 color; };
	struct sprite_queue_entry { int id; double x; double y; };
	
	unsigned int			sprite_program = 0;
	int const				sprite_trigs_max = 21800;
	int						sprite_trigs_count = 0;
	sprite_vtx_format		sprite_trigs_array[sprite_trigs_max * 3];
	unsigned int			sprite_trigs_vao = 0;
	unsigned int			sprite_trigs_vbo = 0;
	std::vector<sprite_queue_entry> sprite_queue;
	std::vector<image>		images;
}

using namespace xs::render::internal;

void xs::render::initialize()
{
	width = configuration::width;
	height = configuration::height;

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
		log::error("DebugRenderer failed to compile vertex shader");
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
		1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_format),
		reinterpret_cast<void*>(offsetof(vertex_format, position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_format),
		reinterpret_cast<void*>(offsetof(vertex_format, color)));

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

void xs::render::render()
{	
	auto w = width / 2.0f;
	auto h = height / 2.0f;
	mat4 p = ortho(-w, w, -h, h, -100.0f, 100.0f);
	mat4 v = lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 vp = p * v;
	
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


	glUseProgram(shader_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));

	for (const auto& spe : sprite_queue)
	{
		const auto& image = images[spe.id];

		const auto from_x = spe.x;
		const auto from_y = spe.y;
		const auto to_x = spe.x + image.width;
		const auto to_y = spe.y + image.height;

		//if (triangles_count < triangles_max - 1)
		{	
			sprite_trigs_array[0].position = { from_x, from_y, 0.0 };
			sprite_trigs_array[1].position = { from_x, to_y, 0.0 };
			sprite_trigs_array[2].position = { to_x, to_y, 0.0 };
			sprite_trigs_array[3].position = { to_x, to_y, 0.0 };
			sprite_trigs_array[4].position = { to_x, from_y, 0.0 };
			sprite_trigs_array[5].position = { from_x, from_y, 0.0 };

			sprite_trigs_array[0].texture = { 0.0, 0.0 };
			sprite_trigs_array[1].texture = { 0.0, 1.0};
			sprite_trigs_array[2].texture = { 1.0, 1.0};
			sprite_trigs_array[3].texture = { 1.0, 1.0};
			sprite_trigs_array[4].texture = { 1.0, 0.0};
			sprite_trigs_array[5].texture = { 0.0, 0.0};

			sprite_trigs_array[0].color = current_color;
			sprite_trigs_array[1].color = current_color;
			sprite_trigs_array[2].color = current_color;
			sprite_trigs_array[3].color = current_color;
			sprite_trigs_array[4].color = current_color;
			sprite_trigs_array[5].color = current_color;
		}

		glBindVertexArray(sprite_trigs_vao);
		glBindBuffer(GL_ARRAY_BUFFER, sprite_trigs_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(sprite_vtx_format) * 6, &sprite_trigs_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, 6);
	}
	XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));

	XS_DEBUG_ONLY(glUseProgram(0));
	XS_DEBUG_ONLY(glBindVertexArray(0));

	// Blit render result screen
	const auto mul = configuration::multiplier;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, render_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width * mul, height * mul, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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

int xs::render::load_image(const std::string& image_file)
{	
	auto buffer = fileio::read_binary_file(image_file);	
	internal::image img;
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
		stbi_image_free(data);
	}

	const auto i = images.size();
	images.push_back(img);
	return static_cast<int>(i);
}

void xs::render::image(int image_id, double x, double y)
{
	sprite_queue.push_back({ image_id, x, y });
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
			assert(lines_begin_count > 1 && lines_count > 1);
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

void xs::render::color(double r, double g, double b, double a)
{
	current_color.r = static_cast<float>(r);
	current_color.g = static_cast<float>(g);
	current_color.b = static_cast<float>(b);
	current_color.a = static_cast<float>(a);
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

void xs::render::poly(double x, double y, double radius, int sides)
{
	const double dt = two_pi<double>() / static_cast<double>(sides);
	double t = 0.0;

	auto x0 = x + radius * cos(t);
	auto y0 = y + radius * sin(t);
	for (int i = 0; i < sides; i++)
	{
		auto x1 = x + radius * cos(t + dt);
		auto y1 = y + radius * sin(t + dt);
		line(x0, y0, x1, y1);
		x0 = x1;
		y0 = y1;
		t += dt;
	}
}

void xs::render::rect(double x, double y, double size_x, double size_y, double rotation)
{
	const uint idx = triangles_count * 3;

	const auto from_x = x - size_x * 0.5;
	const auto from_y = y - size_y * 0.5;
	const auto to_x = x + size_x * 0.5;
	const auto to_y = y + size_y * 0.5;
	
	if (triangles_count < triangles_max - 1)
	{
		triangles_array[idx + 0].position = { from_x, from_y, 0.0 };
		triangles_array[idx + 1].position = { from_x, to_y, 0.0 };
		triangles_array[idx + 2].position = { to_x, to_y, 0.0 };
		triangles_array[idx + 3].position = { to_x, to_y, 0.0 };
		triangles_array[idx + 4].position = { to_x, from_y, 0.0 };
		triangles_array[idx + 5].position = { from_x, from_y, 0.0 };

		triangles_array[idx + 0].color = current_color;
		triangles_array[idx + 1].color = current_color;
		triangles_array[idx + 2].color = current_color;
		triangles_array[idx + 3].color = current_color;
		triangles_array[idx + 4].color = current_color;
		triangles_array[idx + 5].color = current_color;

		triangles_count += 2;
	}
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
