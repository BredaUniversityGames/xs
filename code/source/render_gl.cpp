#include "opengl.h"
#include "render.h"
#include "render_internal.h"
#include "tools.h"
#include <ios>
#include <array>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include <imgui.h>
#include <IconsFontAwesome5.h>


// Include stb_image 
#ifdef PLATFORM_SWITCH
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#include <stb/stb_image.h>
#pragma clang diagnostic pop
#else
#include <stb/stb_image.h>
#endif

#ifdef PLATFORM_SWITCH
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-function"
	#include <stb/stb_easy_font.h>
	#pragma clang diagnostic pop
#else
	#include <stb/stb_easy_font.h>
	#include <filesystem>
#endif

#include "configuration.h"
#include "fileio.h"
#include "log.h"
#include "tools.h"
#include "device.h"
#include "profiler.h"

#define XS_DEBUG_EXTENTS 0

using namespace glm;
using namespace std;

namespace xs::render
{
	void create_frame_buffers();
	void delete_frame_buffers();
	void compile_draw_shader();
	void compile_sprite_shader();
	bool compile_shader(GLuint* shader, GLenum type, const GLchar* source);
	bool link_program(GLuint program);
	void gl_label(GLenum type, GLuint name, const std::string& label);

	int width = -1;
	int height = -1;	

	unsigned int render_fbo;
	unsigned int render_texture;

	unsigned int msaa_fbo;
	unsigned int msaa_texture;
	
	unsigned int			shader_program = 0;
	unsigned int			lines_vao = 0;
	unsigned int			lines_vbo = 0;
	unsigned int			triangles_vao = 0;
	unsigned int			triangles_vbo = 0;

	struct sprite_vtx_format
	{
		vec3 position;
		vec2 texture;
		vec4 add_color;
		vec4 mul_color;
	};	
	
	unsigned int			sprite_program = 0;

	struct sprite_mesh
	{
		unsigned int				vao = 0;
		unsigned int				ebo = 0;
		std::array<unsigned int, 4>	vbos = { 0, 0, 0, 0 };
		uint32_t					count = 0;
		int							image_id = 0;
		tools::aabb					extents;
		bool						is_sprite = false;
	};

	struct sprite_mesh_instance
	{
		int sprite_id = 0;
		int image_id = 0;
		double x = 0;
		double y = 0;
		double z = 0;
		double scale = 1.0;
		double rotation = 0.0;
		glm::vec4 mul_color = glm::vec4(1.0f);
		glm::vec4 add_color = glm::vec4(0.0f);
		uint flags = 0;
	};

	unordered_map<int, sprite_mesh>	sprite_meshes;
	vector<sprite_mesh_instance>	sprite_queue;

	xs::render::stats render_stats = {};
}

using namespace xs;

void xs::render::initialize()
{
	width = configuration::width();
	height = configuration::height();

	create_frame_buffers();
	compile_draw_shader();
	compile_sprite_shader();

	///////// Lines //////////////////////
    glGenVertexArrays(1, &lines_vao);
	glBindVertexArray(lines_vao);

	// Allocate VBO
	glGenBuffers(1, &lines_vbo);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);

	// Allocate into VBO
	const auto size = sizeof(lines_array);
	glBufferData(GL_ARRAY_BUFFER, size, &lines_array[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 4, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, color)));

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
		1, 3, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, color)));

	XS_DEBUG_ONLY(glBindVertexArray(0));
}

void xs::render::shutdown()
{
	// Shutdown the render system in reverse order

	// Trigs
	glDeleteBuffers(1, &triangles_vbo);
	glDeleteVertexArrays(1, &triangles_vao);

	// Lines
	glDeleteBuffers(1, &lines_vbo);
	glDeleteVertexArrays(1, &lines_vao);

	// Shaders
	glDeleteProgram(sprite_program);
	glDeleteProgram(shader_program);

	// Frame buffer
	delete_frame_buffers();

	// Clear the sprite meshes
	sprite_meshes.clear();
	// Clear the sprite queue
	sprite_queue.clear();

	// Clear the fonts	
	fonts.clear();

	// Clear the images
	for (auto& img : images)
		glDeleteTextures(1, &img.texture);
	images.clear();	
}

void xs::render::render()
{	
	XS_PROFILE_SECTION("xs::render::render");
	render_stats = {};

	// Bind MSAA framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
	glViewport(0, 0, width, height);
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	// set the viewport to the screen size

	auto w = width / 2.0f;
	auto h = height / 2.0f;
	mat4 p = ortho(-w, w, -h, h, -100.0f, 100.0f);
	mat4 v = lookAt(vec3(0.0f, 0.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	mat4 vp = p * v;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Clear the screen
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Enable depth testing and set the depth function
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(sprite_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));

	std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
		[](const sprite_mesh_instance& lhs, const sprite_mesh_instance& rhs) {
			if (lhs.z == rhs.z)
				return lhs.sprite_id < rhs.sprite_id;
			return lhs.z < rhs.z;
		});

	for (auto i = 0; i < sprite_queue.size(); i++)
	{
		const auto& spe = sprite_queue[i];
		if(spe.sprite_id == -1) continue;
		auto& mesh = sprite_meshes[spe.sprite_id];
		auto& img = images[mesh.image_id];

		// Set the shader program
		glUseProgram(sprite_program);

		// Set the texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, img.texture);

		// Set the model matrix
		float dx = mesh.extents.max.x - mesh.extents.min.x;
		float dy = mesh.extents.max.y - mesh.extents.min.y;
		mat4 model = identity<mat4>();

		model = translate(model, vec3((float)spe.x, (float)spe.y, 0.0));
		if (!tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::overlay))
			model = translate(model, vec3(offset, 0.0));

		model = rotate(model, (float)spe.rotation, vec3(0.0f, 0.0f, 1.0f));
		model = scale(model, vec3((float)spe.scale, (float)spe.scale, 1.0f));

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_x))
			model = translate(model, vec3(-dx * 0.5f, 0.0f, 0.0f));
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_y))
			model = translate(model, vec3(0.0f, -dy * 0.5f, 0.0f));
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::top))
			model = translate(model, vec3(0.0f, -dy, 0.0f));

		mat4 mvp = vp * model;

		// Get the AABB in view space
		static tools::aabb view_aabb({-1,-1},{1,1});
		tools::aabb bb = mesh.extents.transform(mvp);
#if		XS_DEBUG_EXTENTS
		tools::aabb bb2 = mesh.extents.transform(mv);
		bb2.debug_draw();
#endif
		
		// Check if the sprite is outside the screen
		if (tools::aabb::overlap(bb, view_aabb))
		{
			glUniformMatrix4fv(1, 1, false, value_ptr(mvp));
			glUniform4fv(2, 1, value_ptr(spe.mul_color));
			glUniform4fv(3, 1, value_ptr(spe.add_color));
			glUniform1ui(4, spe.flags);

			// Bind the vertex array
			glBindVertexArray(mesh.vao);

			// Draw the mesh
			glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr);

			// Unbind the vertex array
			glBindVertexArray(0);

			render_stats.draw_calls++;
		}
		else
		{
			if (!bb.is_valid()) {}
		}
	}
	
	glUseProgram(shader_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));

	if (triangles_count > 0)
	{
		glBindVertexArray(triangles_vao);
		glBindBuffer(GL_ARRAY_BUFFER, triangles_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(debug_vertex_format) * triangles_count * 3, &triangles_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, triangles_count * 3);
		XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
		render_stats.draw_calls++;
	}

	if (lines_count > 0)
	{
		glBindVertexArray(lines_vao);
		glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(debug_vertex_format) * lines_count * 2, &lines_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, lines_count * 2);
		XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
		render_stats.draw_calls++;
	}

	XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	XS_DEBUG_ONLY(glUseProgram(0));
	XS_DEBUG_ONLY(glBindVertexArray(0));

	glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, render_fbo);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Blit render result screen
	const auto& screen_to_game = xs::configuration::get_scale_to_game(xs::device::get_width(), xs::device::get_height());
	glBindFramebuffer(GL_READ_FRAMEBUFFER, render_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, screen_to_game.xmin, screen_to_game.ymin, screen_to_game.xmax, screen_to_game.ymax, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	
	render_stats.sprites = (int)sprite_meshes.size();
	render_stats.textures = (int)images.size();
}

void xs::render::render_sprite(
	int sprite_id,
	double x,
	double y,
	double z,
	double size,
	double rotation,
	color multiply,
	color add,
	unsigned int flags)
{
	// Queue the sprite to render
	sprite_mesh_instance instance;
	instance.sprite_id = sprite_id;
	instance.image_id = sprite_meshes[sprite_id].image_id;
	instance.x = x;
	instance.y = y;
	instance.z = z;
	instance.scale = size;
	instance.rotation = rotation;
	instance.mul_color = vec4(
		(float)multiply.r / 255.0f,
		(float)multiply.g / 255.0f,
		(float)multiply.b / 255.0f,
		(float)multiply.a / 255.0f);
	instance.add_color = vec4(
		(float)add.r / 255.0f,
		(float)add.g / 255.0f,
		(float)add.b / 255.0f,
		(float)add.a / 255.0f);
	instance.flags = flags;
	sprite_queue.push_back(instance);
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

void xs::render::create_texture_with_data(xs::render::image& img, uchar* data)
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

	glGenTextures(1, &img.texture);
	glBindTexture(GL_TEXTURE_2D, img.texture);
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

	// Create mipmaps
	glGenerateMipmap(GL_TEXTURE_2D);

	XS_DEBUG_ONLY(glBindTexture(GL_TEXTURE_2D, 0));
}

void xs::render::create_frame_buffers()
{
	//glCreateFramebuffers(1, &render_fbo);
    glGenFramebuffers(1, &render_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glGenTextures(1, &render_texture);
	glBindTexture(GL_TEXTURE_2D, render_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_texture, 0);

	// Create the depth buffer
	unsigned int depth_buffer;
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		assert(false);
	XS_DEBUG_ONLY(glBindFramebuffer(GL_FRAMEBUFFER, 0));

}

void xs::render::delete_frame_buffers()
{
	glDeleteTextures(1, &render_texture);
	glDeleteFramebuffers(1, &render_fbo);

	glDeleteTextures(1, &msaa_texture);
	glDeleteFramebuffers(1, &msaa_fbo);
}

int xs::render::create_sprite(int image_id, double x0, double y0, double x1, double y1)
{
	if(image_id < 0 || image_id >= images.size())
	{
		log::error("Invalid image id: {}", image_id);
		return -1;
	}

	// Precision for the texture coordinates 
	// double precision = 10000.0;
	//int xh0 = (int)(x0 * precision);
	//int yh0 = (int)(y0 * precision);
	//int xh1 = (int)(x1 * precision);
	//int yh1 = (int)(y1 * precision);

	// Check if the sprite already exists
	auto key = tools::hash_combine(image_id, x0, y0, x1, y1);
	auto it = sprite_meshes.find(key);
	if (it != sprite_meshes.end())
		return it->first;

	// Create the sprite mesh
	sprite_mesh mesh;
	mesh.is_sprite = true;

	// Index of the vertices
	unsigned short sprite_indices[] = { 0, 1, 2, 2, 3, 0 };

	// Get the image size
	auto& img = images[image_id];
	auto w = (float)img.width;
	auto h = (float)img.height;

	// Scale the sprite to make sure each pixel is 1.0 units
	float from_x = 0.0f;
	float from_y = 0.0f;
	float to_x = float(w * (x1 - x0));
	float to_y = float(h * (y1 - y0));
	mesh.extents = tools::aabb(vec2(from_x, from_y), vec2(to_x, to_y));
	std::swap(from_y, to_y);

	// Vertex positions just
	float sprite_positions[] = {
		from_x, from_y,
		from_x, to_y,
		to_x, to_y,
		to_x, from_y,
	};

	// Texture coordinates
	float sprite_texture_coordinates[] = {
		(float)x0, (float)y0,
		(float)x0, (float)y1,
		(float)x1, (float)y1,
		(float)x1, (float)y0
	};

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	// Create the index buffer
	glGenBuffers(1, &mesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(short), sprite_indices, GL_STATIC_DRAW);
	mesh.count = 6;

	// Create the vertex buffers
	glGenBuffers(2, mesh.vbos.data());

	// Create the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), sprite_positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the texture coordinate buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), sprite_texture_coordinates, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Unbind the vertex array
	glBindVertexArray(0);

	// Store the mesh
	mesh.image_id = image_id;
	sprite_meshes[key] = mesh;
	return key;
}

int xs::render::create_shape(
	int image_id,
	const float* positions,
	const float* texture_coordinates,
	unsigned int vertex_count,
	const unsigned short* indices,
	unsigned int index_count)
{
	// Create the sprite mesh
	sprite_mesh mesh = {};
	mesh.is_sprite = false;
	auto key = tools::random_id();

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	// Create the index buffer
	glGenBuffers(1, &mesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned short), indices, GL_STATIC_DRAW);
	mesh.count = index_count;

	// Create the vertex buffers
	glGenBuffers(2, mesh.vbos.data());

	// Create the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the texture coordinate buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), texture_coordinates, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Unbind the vertex array
	glBindVertexArray(0);

	// Store the mesh
	mesh.image_id = image_id;
	sprite_meshes[key] = mesh;
	return key;
}

void xs::render::destroy_shape(int sprite_id)
{
	auto it = sprite_meshes.find(sprite_id);
	if (it != sprite_meshes.end())
	{
		auto& mesh = it->second;
		if(!mesh.is_sprite) 
		{
			glDeleteVertexArrays(1, &mesh.vao);
			glDeleteBuffers(1, &mesh.ebo);
			glDeleteBuffers(4, mesh.vbos.data());
			sprite_meshes.erase(it);
		}
	}
}

xs::render::stats xs::render::get_stats()
{
	return render_stats;
}

void xs::render::compile_sprite_shader()
{
	auto vs_str = xs::fileio::read_text_file("[shared]/shaders/sprite.vert");
	auto fs_str = xs::fileio::read_text_file("[shared]/shaders/sprite.frag");
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

void xs::render::compile_draw_shader()
{
	const auto* const vs_source =
		"#version 460 core												\n\
		layout (location = 1) in vec4 a_position;						\n\
		layout (location = 2) in vec4 a_color;							\n\
		layout (location = 1) uniform mat4 u_worldviewproj;				\n\
		out vec4 v_color;												\n\
																		\n\
		void main()														\n\
		{																\n\
			v_color = a_color;											\n\
			gl_Position = u_worldviewproj * a_position, 1.0;			\n\
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

bool xs::render::compile_shader(GLuint* shader, GLenum type, const GLchar* source)
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

bool xs::render::link_program(GLuint program)
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

void xs::render::gl_label(GLenum type, GLuint name, const std::string& label)
{
	std::string typeString;
	switch (type)
	{
	case GL_BUFFER:
		typeString = "GL_BUFFER";
		break;
	case GL_SHADER:
		typeString = "GL_SHADER";
		break;
	case GL_PROGRAM:
		typeString = "GL_PROGRAM";
		break;
	case GL_VERTEX_ARRAY:
		typeString = "GL_VERTEX_ARRAY";
		break;
	case GL_QUERY:
		typeString = "GL_QUERY";
		break;
	case GL_PROGRAM_PIPELINE:
		typeString = "GL_PROGRAM_PIPELINE";
		break;
	case GL_TRANSFORM_FEEDBACK:
		typeString = "GL_TRANSFORM_FEEDBACK";
		break;
	case GL_SAMPLER:
		typeString = "GL_SAMPLER";
		break;
	case GL_TEXTURE:
		typeString = "GL_TEXTURE";
		break;
	case GL_RENDERBUFFER:
		typeString = "GL_RENDERBUFFER";
		break;
	case GL_FRAMEBUFFER:
		typeString = "GL_FRAMEBUFFER";
		break;
	default:
		typeString = "UNKNOWN";
		break;
	}

	const std::string temp = "[" + typeString + ":" + std::to_string(name) + "] " + label;
	glObjectLabel(type, name, static_cast<GLsizei>(temp.length()), temp.c_str());
}