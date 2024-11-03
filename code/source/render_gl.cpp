#include "opengl.h"
#include "render.h"
#include "render_internal.h"
#include "tools.h"
#include "data.h"
#include <array>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <set>
#include <map>
#include <regex>
#include <sstream>

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
#define XS_QUANTIZED_HASHING 0

using namespace glm;
using namespace std;

static const unsigned int c_invalid_index = 4294967295;  // Just -1 casted to unsigned int
static const unsigned int c_instances_ubo_binding = 1;
static const unsigned int c_max_instances = 128;

namespace xs::render
{
	void create_frame_buffers();
	void delete_frame_buffers();

	void compile_draw_shader();
	void compile_sprite_shader();
	void compile_shape_shader();

	bool compile_shader(GLuint* shader, GLenum type, const GLchar* source);
	bool compile_shader(
		const GLchar* vertex_shader,
		const GLchar* geometry_shader,
		const GLchar* fragment_shader,
		GLuint* program);
	bool link_program(GLuint program);

	int width = -1;
	int height = -1;	

	unsigned int render_fbo;
	unsigned int render_texture;

	unsigned int msaa_fbo;
	unsigned int msaa_texture;
	
	unsigned int shader_program = 0;
	unsigned int lines_vao = 0;
	unsigned int lines_vbo = 0;
	unsigned int triangles_vao = 0;
	unsigned int triangles_vbo = 0;

	// Make sure is in sync with the shader
	struct instance_struct
	{
		vec4 mul_color;	// 16   
		vec4 add_color; // 16
		vec2 position;	// 8
		vec2 scale;		// 8
		float rotation;	// 4
		uint flags;		// 4
	};

	instance_struct* instances_data = nullptr;
	unsigned int instances_ubo = c_invalid_index;

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
		vec2						textureFrom;
		vec2						textureTo;
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
		vec4 mul_color = vec4(1.0f);
		vec4 add_color = vec4(0.0f);
		uint flags = 0;
	};

	unordered_map<int, sprite_mesh>	sprite_meshes;
	vector<sprite_mesh_instance>	sprite_queue;

	xs::render::stats render_stats = {};


	class shader_preprocessor
	{
	public:
		shader_preprocessor();
		std::string read(const std::string& path);
	private:
		std::string	parse_recursive(
			const std::string& path,
			const std::string& parent_path,
			std::set<std::string>& include_tree);
		static std::string get_parent_path(const std::string&path);

		std::vector<std::string> m_search_paths;
		std::map<std::string, std::string> m_cached_sources;	
	};
}

using namespace xs;

void xs::render::initialize()
{
	width = configuration::width();
	height = configuration::height();

	create_frame_buffers();
	compile_draw_shader();
	compile_sprite_shader();

	///////// UBO //////////////////////
	instances_data = new instance_struct[c_max_instances];
	glGenBuffers(1, &instances_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, instances_ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(instance_struct) * c_max_instances, instances_data, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, c_instances_ubo_binding, instances_ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	xs::gl_label(GL_BUFFER, instances_ubo, "Instances");

	///////// Lines //////////////////////
    glGenVertexArrays(1, &lines_vao);
	glBindVertexArray(lines_vao);

	// Allocate VBO
	glGenBuffers(1, &lines_vbo);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);

	// Allocate into VBO
	constexpr auto size = sizeof(lines_array);
	glBufferData(GL_ARRAY_BUFFER, size, &lines_array[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 4, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, position)));  // NOLINT(performance-no-int-to-ptr)

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, color)));  // NOLINT(performance-no-int-to-ptr)

	XS_DEBUG_ONLY(glBindVertexArray(0));

	///////// Trigs //////////////////////
	glGenVertexArrays(1, &triangles_vao);
	glBindVertexArray(triangles_vao);

	// Allocate VBO
	glGenBuffers(1, &triangles_vbo);

	// Array buffer contains the attribute data
	glBindBuffer(GL_ARRAY_BUFFER, triangles_vbo);

	// Allocate into VBO
	constexpr auto trigs_size = sizeof(triangles_array);
	glBufferData(GL_ARRAY_BUFFER, trigs_size, &triangles_array[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, position)));  // NOLINT(performance-no-int-to-ptr)

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(debug_vertex_format),
		reinterpret_cast<void*>(offsetof(debug_vertex_format, color)));  // NOLINT(performance-no-int-to-ptr)

	XS_DEBUG_ONLY(glBindVertexArray(0));

#ifdef DEBUG
	gl_label(GL_VERTEX_ARRAY, lines_vao, "lines vao");
	gl_label(GL_VERTEX_ARRAY, triangles_vao, "triangles vao");
	gl_label(GL_BUFFER, lines_vbo, "lines vbo");
	gl_label(GL_BUFFER, triangles_vbo, "triangles vbo");
#endif

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

	// Disable depth testing and face culling
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(sprite_program);
	glUniformMatrix4fv(0, 1, false, value_ptr(vp));

	std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
		[](const sprite_mesh_instance& lhs, const sprite_mesh_instance& rhs) {		
			return lhs.z < rhs.z;
		});
	
	for (const auto& spe : sprite_queue)
	{
		if (spe.sprite_id == -1) continue;
		auto& mesh = sprite_meshes[spe.sprite_id];
		auto& img = images[mesh.image_id];

		// Set the model matrix
		float dx = mesh.extents.max.x - mesh.extents.min.x;
		float dy = mesh.extents.max.y - mesh.extents.min.y;

		// Set the texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, img.texture);

		instances_data[0].mul_color = spe.mul_color;
		instances_data[0].add_color = spe.add_color;
		instances_data[0].position = vec2((float)spe.x, (float)spe.y);
		instances_data[0].scale = vec2((float)spe.scale, (float)spe.scale);
		instances_data[0].rotation = (float)spe.rotation;
		instances_data[0].flags = spe.flags;

		glBindBuffer(GL_UNIFORM_BUFFER, instances_ubo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(instance_struct), &instances_data[0]);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);


		// Bind the vertex array
		glBindVertexArray(mesh.vao);

		// Draw the mesh
		glDrawElementsInstanced(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr, 1);

		// Unbind the vertex array
		XS_DEBUG_ONLY(glBindVertexArray(0));

		render_stats.draw_calls++;
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

	bool filter_flag = data::get_bool("Texture Filter", data::type::project);
	auto filter = filter_flag ? GL_LINEAR : GL_NEAREST;

	bool repeat_flag = data::get_bool("Texture Repeat", data::type::project);
	auto repeat = repeat_flag ? GL_REPEAT : GL_CLAMP_TO_EDGE;

	glGenTextures(1, &img.texture);
	glBindTexture(GL_TEXTURE_2D, img.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);

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

	gl_label(GL_TEXTURE, img.texture, img.file);
	XS_DEBUG_ONLY(glBindTexture(GL_TEXTURE_2D, 0));
}

void xs::render::create_frame_buffers()
{
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

	gl_label(GL_FRAMEBUFFER, render_fbo, "render fbo");
	gl_label(GL_TEXTURE, render_texture, "render texture");
	gl_label(GL_RENDERBUFFER, depth_buffer, "depth buffer");
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
	if(image_id < 0 || image_id >= (int)images.size())
	{
		log::error("Invalid image id: {}", image_id);
		return -1;
	}

#if XS_QUANTIZED_HASHING
	// Precision for the texture coordinates 
	double precision = 10000.0;
	int xh0 = (int)(x0 * precision);
	int yh0 = (int)(y0 * precision);
	int xh1 = (int)(x1 * precision);
	int yh1 = (int)(y1 * precision);
#endif

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
	float to_x = static_cast<float>(w * (x1 - x0));
	float to_y = static_cast<float>(h * (y1 - y0));
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

	string name = "sprite " + img.file + " " + to_string(key);
	gl_label(GL_VERTEX_ARRAY, mesh.vao, name + "vao");
	gl_label(GL_BUFFER, mesh.ebo, name + "ebo");
	gl_label(GL_BUFFER, mesh.vbos[0], name + " position vbo");
	gl_label(GL_BUFFER, mesh.vbos[1], name + " texture vbo");

	// Unbind the vertex array
	XS_DEBUG_ONLY(glBindVertexArray(0));

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

bool xs::render::compile_shader(
	const GLchar* vertex_shader,
	const GLchar* geometry_shader,
	const GLchar* fragment_shader,
	GLuint* program)
{
	GLuint vert_shader = 0;
	GLuint geom_shader = 0;
	GLuint frag_shader = 0;

	*program = glCreateProgram();

	GLboolean res = compile_shader(&vert_shader, GL_VERTEX_SHADER, vertex_shader);
	if (!res)
	{
		log::error("Renderer failed to compile vertex shader");
		return false;
	}

	if (geometry_shader)
	{
		res = compile_shader(&geom_shader, GL_GEOMETRY_SHADER, geometry_shader);
		if (!res)
		{
			log::error("Renderer failed to compile geometry shader");
			return false;
		}
	}

	res = compile_shader(&frag_shader, GL_FRAGMENT_SHADER, fragment_shader);
	if (!res)
	{
		log::error("Renderer failed to compile fragment shader");
		return false;
	}

	glAttachShader(*program, vert_shader);
	if (geometry_shader)
		glAttachShader(*program, geom_shader);
	glAttachShader(*program, frag_shader);

	if (!link_program(*program))
	{
		glDeleteShader(vert_shader);
		if (geometry_shader)
			glDeleteShader(geom_shader);
		glDeleteShader(frag_shader);
		glDeleteProgram(*program);
		log::error("Renderer failed to link shader program");
		return false;
	}

	glDeleteShader(vert_shader);
	if (geometry_shader)
		glDeleteShader(geom_shader);
	glDeleteShader(frag_shader);

	return true;
}

void xs::render::compile_sprite_shader()
{
	auto shader_preprocessor = render::shader_preprocessor();
	auto vs_str = shader_preprocessor.read("[shared]/shaders/sprite.vert");
	auto fs_str = shader_preprocessor.read("[shared]/shaders/sprite.frag");
	const char* const vs_source = vs_str.c_str();
	const char* const fs_source = fs_str.c_str();
	bool success = compile_shader(
		vs_source,
		nullptr,
		fs_source,
		&sprite_program);
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

	bool sucess = compile_shader(
		vs_source,
		nullptr,
		fs_source,
		&shader_program);

	if (!sucess)
		log::error("Renderer failed to compile draw shader");
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


#define ENABLE_PROFILING 0

#if ENABLE_PROFILING
#endif

using namespace std;

namespace {
	const regex sIncludeRegex = regex("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
} // anonymous namespace


xs::render::shader_preprocessor::shader_preprocessor()
{
	m_search_paths.emplace_back("");
}

string xs::render::shader_preprocessor::read(const string& path)
{
	set<string> includeTree;
	return parse_recursive(path, "", includeTree);
}

// Based on
// https://www.opengl.org/discussion_boards/showthread.php/169209-include-in-glsl
string xs::render::shader_preprocessor::parse_recursive(
	const string& path,
	const string& parent_path,
	set<string>& include_tree)
{
	string fullPath = parent_path.empty() ? path : parent_path + "/" + path;

	if (include_tree.count(fullPath))
	{
		log::warn("Circular include found! Path: {}", path);
		return string();
	}

	include_tree.insert(fullPath);	

	fullPath = fileio::get_path(fullPath);
	string parent = get_parent_path(fullPath);
	string inputString =  fileio::read_text_file(fullPath);
	if(inputString.empty())
	{
		log::error("Shader file not found! Path: {}", fullPath);
		return string();
	}

	stringstream input(move(inputString));
	stringstream output;

	// go through each line and process includes
	string line;
	smatch matches;
	size_t lineNumber = 1;
	while (getline(input, line))
	{
		if (regex_search(line, matches, sIncludeRegex))
		{
			output << parse_recursive(matches[1].str(), parent, include_tree);
			output << "#line " << lineNumber << '\n';
		}
		else if(xs::tools::string_starts_with(line,"#extension GL_GOOGLE_include_directive : require"))
		{
			output << '\n';
		}
		else
		{
			if(!line.empty() && line[0] != '\0') // Don't null terminate
				output << line;
		}
		output << '\n';
		lineNumber++;
	}
	
	return output.str();

}

string xs::render::shader_preprocessor::get_parent_path(const string& path)
{
	// Implementation base on:
	// http://stackoverflow.com/questions/28980386/how-to-get-file-name-from-a-whole-path-by-c
	string parent;
	string::size_type found = path.find_last_of("/");  // NOLINT(performance-faster-string-find)

	// if we found one of this symbols
	if (found != string::npos)
		parent = path.substr(0, found);
	
	return parent;
}

