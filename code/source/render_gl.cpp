#include "opengl.h"
#include "render.h"
#include "render_internal.h"
#include <ios>
#include <array>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>

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

using namespace glm;
using namespace std;

namespace xs::render::internal
{
	void create_frame_buffers();
	void create_shadowmap_buffers();

	//TODO: void delete_frame_buffers();
	//TODO: void delete_shadowmap_buffers();
	void compile_draw_shader();
	void compile_sprite_shader();
	void compile_3d_shader();
	void compile_shadow_shader();
	bool compile_shader(GLuint* shader, GLenum type, const GLchar* source);
	bool link_program(GLuint program);

	void render_3d();
	void render_shadows_maps();

	int width = -1;
	int height = -1;	

	unsigned int render_fbo;
	unsigned int render_texture;

	static const int max_dir_lights = 4;  // In sync with .glsl
	static const unsigned int c_invalid_index = 4294967295;  // Just -1 casted to unsigned int
	const int shadow_resolution = 2048;

	
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
	int const				sprite_trigs_max = 21800;
	int						sprite_trigs_count = 0;
	sprite_vtx_format		sprite_trigs_array[sprite_trigs_max * 3];
	unsigned int			sprite_trigs_vao = 0;
	unsigned int			sprite_trigs_vbo = 0;

	struct aabb
	{
		glm::vec2 min = glm::vec2(numeric_limits<float>::max(), numeric_limits<float>::max());
		glm::vec2 max = glm::vec2(numeric_limits<float>::min(), numeric_limits<float>::min());

		aabb() = default;

		aabb(const glm::vec2& min, const glm::vec2& max) : min(min), max(max) {}

		bool is_valid() const { return min.x <= max.x && min.y <= max.y; }

		void add_point(const glm::vec2& point) { min = glm::min(min, point); max = glm::max(max, point); }

		static bool overlap(const aabb& a, const aabb& b)
		{ return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y; }

		aabb transform(const glm::mat4& m) const
		{
			glm::vec2 p[4] = {
				glm::vec2(m * glm::vec4(min.x, min.y, 1.0f, 1.0f)),
				glm::vec2(m * glm::vec4(min.x, max.y, 1.0f, 1.0f)),
				glm::vec2(m * glm::vec4(max.x, max.y, 1.0f, 1.0f)),
				glm::vec2(m * glm::vec4(max.x, min.y, 1.0f, 1.0f))
			};
			aabb result(p[0], p[0]);
			for (int i = 1; i < 4; i++)
				result.add_point(p[i]);
			return result;
		}

		void debug_draw()
		{
			color c = { 255, 255, 255, 255 };
			xs::render::set_color(c);

			// Draw the AABB as a line loop
			debug_vertex_format verts[4] = {
				{glm::vec4(min.x, min.y, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},
				{glm::vec4(min.x, max.y, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},
				{glm::vec4(max.x, max.y, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},
				{glm::vec4(max.x, min.y, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)}
			};

			// Draw the AABB
			for (int i = 0; i < 4; i++)
			{
				xs::render::line(verts[i].position.x, verts[i].position.y, verts[(i + 1) % 4].position.x, verts[(i + 1) % 4].position.y);
			}
		}
	};

	struct directional_light_shadow
	{
		unsigned int shadow_map = 0;
		unsigned int shadow_map_fbo = 0;
		mat4 shadow_matrix = {};
	};

	struct mesh
	{
		unsigned int				vao = 0;
		unsigned int				ebo = 0;
		std::array<unsigned int, 4>	vbos = { 0, 0, 0, 0 };
		uint32_t					count = 0;
	};

	struct sprite_mesh
	{
		unsigned int				vao = 0;
		unsigned int				ebo = 0;
		std::array<unsigned int, 4>	vbos = { 0, 0, 0, 0 };
		uint32_t					count = 0;
		int							image_id = 0;
		//glm::vec4					extents = glm::vec4(0.0f);
		aabb						extents;
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
		int flags = 0;
	};

	//	std::unordered_map<int, mesh>	meshes; TODO: Implement this
	std::vector<mesh>		meshes;
	std::array<directional_light_shadow, max_dir_lights> dir_lights_shadows;
	unsigned int			mesh_program = 0;
	unsigned int			shadow_program = 0;

	glm::mat4						view;
	glm::mat4						projection;
	std::vector<mesh_queue_entry>	mesh_queue;

	unordered_map<int, sprite_mesh>	sprite_meshes;
	vector<sprite_mesh_instance>	sprite_queue2;
}

using namespace xs;
using namespace xs::render::internal;

void xs::render::initialize()
{
	width = configuration::width();
	height = configuration::height();

	internal::create_frame_buffers();
	internal::create_shadowmap_buffers();

	internal::compile_draw_shader();
	internal::compile_sprite_shader();
	internal::compile_3d_shader();
	internal::compile_shadow_shader();

	///////// Trigs //////////////////////
	//glCreateVertexArrays(1, &lines_vao);
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

void xs::render::render()
{	
	XS_PROFILE_SECTION("xs::render::render");

	// render_shadows_maps();
	// render_3d();

	// Setthe render target
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// set the viewport to the screen size
	glViewport(0, 0, width, height);


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

	std::stable_sort(sprite_queue2.begin(), sprite_queue2.end(),
		[](const sprite_mesh_instance& lhs, const sprite_mesh_instance& rhs) {
			return lhs.z < rhs.z;
		});

	for (auto i = 0; i < sprite_queue2.size(); i++)
	{
		const auto& spe = sprite_queue2[i];
		//const auto& sprite = sprite_meshes[spe.sprite_id];
		//const auto& image = images[sprite.image_id];

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

		if(tools::check_bit_flag_overlap(spe.flags, xs::render::flip_x))
			model = scale(model, vec3(-1.0f, 1.0f, 1.0f));
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::flip_y))
			model = scale(model, vec3(1.0f, -1.0f, 1.0f));

		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_x))
			model = translate(model, vec3(-dx * 0.5f, 0.0f, 0.0f));
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_y))
			model = translate(model, vec3(0.0f, -dy * 0.5f, 0.0f));

		model = rotate(model, (float)spe.rotation, vec3(0.0f, 0.0f, 1.0f));
		model = scale(model, vec3((float)spe.scale, (float)spe.scale, 1.0f));
		mat4 mvp = vp * model;
		mat4 mv = v * model;

		// Get the AABB in view space
		static aabb view_aabb({-1,-1},{1,1});
		aabb bb = mesh.extents.transform(mvp);
		// aabb bb2 = mesh.extents.transform(mv);
		// bb2.debug_draw();
		
		// Check if the sprite is outside the screen
		if (aabb::overlap(bb, view_aabb))
		{
			glUniformMatrix4fv(1, 1, false, value_ptr(mvp));
			glUniform4fv(2, 1, value_ptr(spe.mul_color));
			glUniform4fv(3, 1, value_ptr(spe.add_color));

			// Bind the vertex array
			glBindVertexArray(mesh.vao);

			// Draw the mesh
			glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr);

			// Unbind the vertex array
			glBindVertexArray(0);
		}	


		/*
		// Create the AABB
		vec2 min = vec2(mesh.extents.x, mesh.extents.y);
		vec2 max = vec2(mesh.extents.z, mesh.extents.w);

		// Transform the AABB
		vec4 minp = mvp * vec4(min.x, min.y, 1.0f, 1.0f);
		vec4 maxp = mvp * vec4(max.x, min.y, 1.0f, 1.0f);
		minp /= minp.w;
		maxp /= maxp.w;
		if (minp.x > maxp.x)
			std::swap(minp.x, maxp.x);
		if (minp.y > maxp.y)
			std::swap(minp.y, maxp.y);

		// Check if the sprite is outside the screen
		float aperture = 1.1f;
		if ((abs(minp.x) <= aperture && abs(minp.y) <= aperture) ||
			(abs(maxp.x) <= aperture && abs(maxp.y) <= aperture))
		{
			// Set the uniforms
			glUniformMatrix4fv(1, 1, false, value_ptr(mvp));
			glUniform4fv(2, 1, value_ptr(spe.mul_color));
			glUniform4fv(3, 1, value_ptr(spe.add_color));

			// Bind the vertex array
			glBindVertexArray(mesh.vao);

			// Draw the mesh
			glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr);

			// Unbind the vertex array
			glBindVertexArray(0);
		}
		*/
	}

	/*

	std::stable_sort(sprite_queue.begin(), sprite_queue.end(),
		[](const sprite_queue_entry& lhs, const sprite_queue_entry& rhs) {
			return lhs.z < rhs.z;
		});

	int count = 0;
	for (auto i = 0; i < sprite_queue.size(); i++)
	{
		const auto& spe = sprite_queue[i];
		const auto& sprite = sprites[spe.sprite_id];
		const auto& image = images[sprite.image_id];

		auto from_x = 0.0;
		auto from_y = 0.0;
		auto to_x = image.width * (sprite.to.x - sprite.from.x) * spe.scale;
		auto to_y = image.height * (sprite.to.y - sprite.from.y) * spe.scale;

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
		glBindTexture(GL_TEXTURE_2D, image.texture);

		sprite_trigs_array[count + 0].position = { from_x, from_y, 0.0 };
		sprite_trigs_array[count + 1].position = { from_x, to_y, 0.0 };
		sprite_trigs_array[count + 2].position = { to_x, to_y, 0.0 };
		sprite_trigs_array[count + 3].position = { to_x, to_y, 0.0 };
		sprite_trigs_array[count + 4].position = { to_x, from_y, 0.0 };
		sprite_trigs_array[count + 5].position = { from_x, from_y, 0.0 };

		sprite_trigs_array[count + 0].texture = { from_u,	to_v };
		sprite_trigs_array[count + 1].texture = { from_u,	from_v };
		sprite_trigs_array[count + 2].texture = { to_u,		from_v };
		sprite_trigs_array[count + 3].texture = { to_u,		from_v };
		sprite_trigs_array[count + 4].texture = { to_u,		to_v };
		sprite_trigs_array[count + 5].texture = { from_u,	to_v };

		for (int i = 0; i < 6; ++i)
		{
			sprite_trigs_array[count + i].add_color = add_color;
			sprite_trigs_array[count + i].mul_color = mul_color;
		}
			
		vec3 anchor(0.0f, 0.0f, 0.0f);
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_x))
			anchor.x = (float)((to_x - from_x) * 0.5);
		if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::center_y))
			anchor.y = (float)((to_y - from_y) * 0.5);
		else if (tools::check_bit_flag_overlap(spe.flags, xs::render::sprite_flags::top))
			anchor.y = (float)(to_y - from_y);

		if (anchor.x != 0.0f || anchor.y != 0.0f)
		{				
			for (int i = 0; i < 6; i++)
				sprite_trigs_array[count + i].position -= anchor;
		}

		if (spe.rotation != 0.0)
		{
			for (int i = 0; i < 6; i++)
				rotate_vector3d(sprite_trigs_array[count + i].position, (float)spe.rotation);
		}

		for (int i = 0; i < 6; i++)
		{
			sprite_trigs_array[count + i].position.x += (float)spe.x;
			sprite_trigs_array[count + i].position.y += (float)spe.y;
		}
		count += 6;

		bool render_batch = false;
		if (i < sprite_queue.size() - 1)
		{
			const auto& nspe = sprite_queue[i + 1];
			const auto& nsprite = sprites[nspe.sprite_id];
			render_batch = nsprite.image_id != sprite.image_id;
		}
		else
		{
			render_batch = true;
		}
		if (render_batch)
		{
			glBindVertexArray(sprite_trigs_vao);
			glBindBuffer(GL_ARRAY_BUFFER, sprite_trigs_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(sprite_vtx_format) * count, &sprite_trigs_array[0], GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLES, 0, count);
			count = 0;
		}
	}

	XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	XS_DEBUG_ONLY(glUseProgram(0));
	XS_DEBUG_ONLY(glBindVertexArray(0));
	*/
	
	glUseProgram(shader_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));

	if (triangles_count > 0)
	{
		glBindVertexArray(triangles_vao);
		glBindBuffer(GL_ARRAY_BUFFER, triangles_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(debug_vertex_format) * triangles_count * 3, &triangles_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, triangles_count * 3);
		XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}

	if (lines_count > 0)
	{
		glBindVertexArray(lines_vao);
		glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(debug_vertex_format) * lines_count * 2, &lines_array[0], GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, lines_count * 2);
		XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}

	XS_DEBUG_ONLY(glBindBuffer(GL_ARRAY_BUFFER, 0));
	XS_DEBUG_ONLY(glUseProgram(0));
	XS_DEBUG_ONLY(glBindVertexArray(0));

	// Blit render result screen
	const auto& screen_to_game = xs::configuration::get_scale_to_game(xs::device::get_width(), xs::device::get_height());

	glBindFramebuffer(GL_READ_FRAMEBUFFER, render_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, screen_to_game.xmin, screen_to_game.ymin, screen_to_game.xmax, screen_to_game.ymax, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int xs::render::create_sprite2(int image_id, double x0, double y0, double x1, double y1)
{
	// Precision for the texture coordinates 
	double precision = 10000.0;
	int xh0 = (int)(x0 * precision);
	int yh0 = (int)(y0 * precision);
	int xh1 = (int)(x1 * precision);
	int yh1 = (int)(y1 * precision);

	// Check if the sprite already exists
	auto key = (int)tools::hash_combine(image_id, xh0, yh0, xh1, yh1);
	auto it = sprite_meshes.find(key);
	if (it != sprite_meshes.end())
		return it->first;

	// Create the sprite mesh
	sprite_mesh mesh;

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
	mesh.extents = aabb(vec2(from_x, from_y), vec2(to_x, to_y));
	std::swap(from_y, to_y);

	// Vertex positions just
	float sprite_positions[] = {
		from_x, from_y, 0.0f,
		from_x, to_y, 0.0f,
		to_x, to_y, 0.0f,
		to_x, from_y, 0.0f
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
	glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float), sprite_positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

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

void xs::render::render_sprite2(
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
	sprite_queue2.push_back(instance);
}

void xs::render::render_shape(
	int shape_id,
	double x,
	double y,
	double z,
	double size,
	double rotation,
	color mutiply,
	color add,
	unsigned int flags)
{
	render_sprite2(shape_id, x, y, z, size, rotation, mutiply, add, flags);
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
	sprite_queue2.clear();
}

void xs::render::internal::create_texture_with_data(xs::render::internal::image& img, uchar* data)
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

	XS_DEBUG_ONLY(glBindTexture(GL_TEXTURE_2D, 0));
}

void xs::render::internal::create_frame_buffers()
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

void xs::render::internal::create_shadowmap_buffers()
{
	for (int i = 0; i < internal::max_dir_lights; i++)
	{
		// Create the shadow map texture
		glGenTextures(1, &internal::dir_lights_shadows[i].shadow_map);
		glBindTexture(GL_TEXTURE_2D, internal::dir_lights_shadows[i].shadow_map);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, internal::shadow_resolution, internal::shadow_resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, value_ptr(vec4(1.0f)));

		// Create the shadow map frame buffer
		glGenFramebuffers(1, &internal::dir_lights_shadows[i].shadow_map_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, internal::dir_lights_shadows[i].shadow_map_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, internal::dir_lights_shadows[i].shadow_map, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			assert(false);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

int xs::render::create_mesh(
	const unsigned int* indices,
	unsigned int index_count,
	const float* positions,
	const float* normals,
	const float* texture_coordinates,
	const float* colors,
	unsigned int vertex_count)
{
	// Create an OpenGL mesh
	internal::mesh mesh;
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	// Create the index buffer
	glGenBuffers(1, &mesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(int), indices, GL_STATIC_DRAW);
	mesh.count = index_count;

	// Create the vertex buffers	
	glGenBuffers((GLsizei)mesh.vbos.size(), mesh.vbos.data());
	//glGenBuffers(1, mesh.vbos.data());

	// Create the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Create the normal buffer
	if (normals)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), normals, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	}

	// Create the texture coordinate buffer
	if (texture_coordinates)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), texture_coordinates, GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	}

	// Create the color buffer
	if (colors)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[3]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), colors, GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	}

	// Unbind the vertex array
	glBindVertexArray(0);

	// Store the mesh
	internal::meshes.push_back(mesh);
	return (int)internal::meshes.size();
}

int xs::render::create_shape(
	int image_id,
	const float* positions,
	const float* texture_coordinates,
	unsigned int vertex_count,
	const unsigned int* indices,
	unsigned int index_count)
{
	// Create the sprite mesh
	sprite_mesh mesh;

	auto key = (int)tools::hash_combine(image_id, positions, texture_coordinates, vertex_count, indices, index_count);

		/*
	// Check if the sprite already exists
	auto key = (int)tools::hash_combine({
		image_id,
		reinterpret_cast<int>(positions),
		reinterpret_cast<int>(texture_coordinates),
		(int)(vertex_count),
		reinterpret_cast<int>(indices),
		(int)(index_count) });
		*/
	auto it = sprite_meshes.find(key);
	if (it != sprite_meshes.end())
		return it->first;

	// Calculate the extents
	for (unsigned int i = 0; i < vertex_count; i++)
	{
		vec2 p(positions[i * 2 + 0], positions[i * 3 + 1]);
		mesh.extents.add_point(p);
	}


	/*

	// Get the image size
	auto& img = images[image_id];
	auto w = (float)img.width;
	auto h = (float)img.height;

	// Scale the sprite to make sure each pixel is 1.0 units
	float from_x = 0.0f;
	float from_y = 0.0f;
	float to_x = float(w * (x1 - x0));
	float to_y = float(h * (y1 - y0));
	mesh.extents = aabb(vec2(from_x, from_y), vec2(to_x, to_y));
	std::swap(from_y, to_y);

	// Vertex positions just
	float sprite_positions[] = {
		from_x, from_y, 0.0f,
		from_x, to_y, 0.0f,
		to_x, to_y, 0.0f,
		to_x, from_y, 0.0f
	};

	// Texture coordinates
	float sprite_texture_coordinates[] = {
		(float)x0, (float)y0,
		(float)x0, (float)y1,
		(float)x1, (float)y1,
		(float)x1, (float)y0
	};
	*/

	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	// Create the index buffer
	glGenBuffers(1, &mesh.ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(short), indices, GL_STATIC_DRAW);
	mesh.count = index_count;

	// Create the vertex buffers
	glGenBuffers(2, mesh.vbos.data());

	// Create the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), positions, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

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

void xs::render::render_mesh(
	int mesh_id,
	int image_id,
	const glm::mat4& transform,
	const glm::vec4& mul_color,
	const glm::vec4& add_color)
{
	// Check that the mesh handle is valid
	if (mesh_id < 0 || mesh_id >(int)meshes.size())
		return;

	// Check that the texture handle is valid
	if (image_id < 0 || image_id >(int)images.size())
		return;

	// Add to the list of meshes to render
	mesh_queue_entry instance;
	instance.transform = transform;
	instance.mesh = mesh_id;
	instance.texture = image_id;
	instance.mul_color = mul_color;
	instance.add_color = add_color;
	mesh_queue.push_back(instance);
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

void xs::render::internal::compile_3d_shader()
{
	auto vs_str = xs::fileio::read_text_file("[games]/shared/shaders/standard.vert");
	auto fs_str = xs::fileio::read_text_file("[games]/shared/shaders/standard.frag");
	const char* const vs_source = vs_str.c_str();
	const char* const fs_source = fs_str.c_str();

	GLuint vert_shader = 0;
	GLuint frag_shader = 0;

	mesh_program = glCreateProgram();

	GLboolean res = compile_shader(&vert_shader, GL_VERTEX_SHADER, vs_source);
	if (!res)
	{
		log::error("Renderer failed to compile standard vertex shader");
		return;
	}

	res = compile_shader(&frag_shader, GL_FRAGMENT_SHADER, fs_source);
	if (!res)
	{
		log::error("Renderer failed to compile standard fragment shader");
		return;
	}

	glAttachShader(mesh_program, vert_shader);
	glAttachShader(mesh_program, frag_shader);

	if (!link_program(mesh_program))
	{
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
		glDeleteProgram(mesh_program);
		log::error("Renderer failed to link standard shader program");
		return;
	}

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
}

void xs::render::internal::compile_shadow_shader()
{
	const auto* const vs_source =
		"#version 460 core												\n\
		layout (location = 0) in vec4 a_position;						\n\
		layout (location = 1) uniform mat4 u_worldviewproj;				\n\
																		\n\
		void main()														\n\
		{																\n\
			gl_Position = u_worldviewproj * a_position, 1.0;			\n\
		}";

	GLuint vert_shader = 0;

	shadow_program = glCreateProgram();

	GLboolean res = compile_shader(&vert_shader, GL_VERTEX_SHADER, vs_source);
	if (!res)
	{
		log::error("Renderer failed to compile shadow vertex shader");
		return;
	}

	glAttachShader(shadow_program, vert_shader);

	if (!link_program(shadow_program))
	{
		glDeleteShader(vert_shader);
		glDeleteProgram(shadow_program);
		log::error("Renderer failed to link shadow shader program");
		return;
	}

	glDeleteShader(vert_shader);
}

void xs::render::internal::compile_draw_shader()
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

void xs::render::internal::render_shadows_maps()
{
	// Go over all lights and render the shadow maps
	for(int i = 0; i < dir_lights.size(); i++)
	{
		auto& shadow = dir_lights_shadows[i];
		const auto& light = dir_lights[i];
		const auto& shadow_map_fbo = shadow.shadow_map_fbo;
		const auto& shadow_map = shadow.shadow_map;

		// Set the viewport to the shadow map resolution
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);
		glViewport(0, 0, shadow_resolution, shadow_resolution);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Activate the shadow program
		glUseProgram(shadow_program);

		// Set the projection matrix
		const auto projection = ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
		const auto view = glm::lookAt(vec3(-light.direction), vec3(0.0f), vec3(0.0f, 0.0f, 1.0f));
		const auto vp = projection * view;
		shadow.shadow_matrix = vp;
		
		// Render each entry
		for (const auto& entry : mesh_queue)
		{
			const auto& mesh = meshes[entry.mesh - 1];
			const auto& image = images[entry.texture];
			const auto& transform = entry.transform;
			auto mvp = vp * transform;

			// Bind the vertex array
			glBindVertexArray(mesh.vao);

			// Send the model matrix to the standard program
			glUniformMatrix4fv(glGetUniformLocation(shadow_program, "u_worldviewproj"), 1, GL_FALSE, value_ptr(mvp));

			// Draw the mesh
			glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, 0);
		}
	}
}

void xs::render::internal::render_3d()
{
	// Set the viewport to the screen resolution
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glViewport(0, 0, width, height);

	// Clear the screen
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	// Enable depth testing and set the depth function
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Activate the standard program
	const auto program = internal::mesh_program;
	glUseProgram(program);

	// Send the view and projection matrices to the standard program
	glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, value_ptr(internal::view));
	glUniformMatrix4fv(glGetUniformLocation(program, "u_projection"), 1, GL_FALSE, value_ptr(projection));
	// auto texture_location = glGetUniformLocation(program, "u_texture");	

	auto texture_location = 1;

	// Go over all the directional lights and set shadow maps (even if they are not used)
	for(int i = 0; i < internal::max_dir_lights; i++)
	{
		const auto& shadow = internal::dir_lights_shadows[i];
		const auto& shadow_map = shadow.shadow_map;

		// Set the shadow map
		int sampler = 10 + i;
		glActiveTexture(GL_TEXTURE0 + sampler);
		glBindTexture(GL_TEXTURE_2D, shadow_map);
		//auto s_shadow_maps = glGetUniformLocation(program, "s_shadow_maps");
		glUniform1i(sampler, sampler);
	}

	// Send the directional lights to the standard program
	glUniform1i(glGetUniformLocation(program, "u_num_directional_lights"), (int)internal::dir_lights.size());
	for (int i = 0; i < internal::dir_lights.size(); ++i)
	{
		std::string name = "u_directional_lights[" + std::to_string(i) + "].direction";
		glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, value_ptr(internal::dir_lights[i].direction));
		name = "u_directional_lights[" + std::to_string(i) + "].color";
		glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &internal::dir_lights[i].color.x);

		name = "u_directional_lights[" + std::to_string(i) + "].cast_shadow";
		glUniform1i(glGetUniformLocation(program, name.c_str()), 1);

		name = "u_directional_lights[" + std::to_string(i) + "].shadow_matrix";		
		const auto& shadow = internal::dir_lights_shadows[i];		
		glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, value_ptr(shadow.shadow_matrix));
	}

	/*
	// Send the point lights to the standard program	
	glUniform1i(glGetUniformLocation(internal::standard_program, "u_num_point_lights"), (int)internal::point_lights.size());
	for (int i = 0; i < internal::point_lights.size(); ++i)
	{
		std::string name = "u_point_lights[" + std::to_string(i) + "]";
		glUniform3fv(glGetUniformLocation(internal::standard_program, (name + ".position").c_str()),
			1, &internal::point_lights[i].position.x);
		glUniform1f(glGetUniformLocation(internal::standard_program, (name + ".radius").c_str()),
			internal::point_lights[i].range);
		glUniform3fv(glGetUniformLocation(internal::standard_program, (name + ".color").c_str()),
			1, &internal::point_lights[i].color.x);
	}
	*/

	// Render each entry
	for (const auto& entry : internal::mesh_queue)
	{
		const auto& mesh = internal::meshes[entry.mesh - 1];
		const auto& image = internal::images[entry.texture];
		// const auto& transform = entry.transform;

		// Bind the vertex array
		glBindVertexArray(mesh.vao);

		// Bind the texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, image.texture);
		// texture_location
		glUniform1i(texture_location, 0);

		// Send the model matrix to the standard program
		glUniformMatrix4fv(glGetUniformLocation(program, "u_model"), 1, GL_FALSE, value_ptr(entry.transform));

		// Send the multiply and add colors to the standard program
		glUniform4fv(glGetUniformLocation(program, "u_mul_color"), 1, value_ptr(entry.mul_color));
		glUniform4fv(glGetUniformLocation(program, "u_add_color"), 1, value_ptr(entry.add_color));

		// Draw the mesh
		glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, 0);
	}

	mesh_queue.clear();
	dir_lights.clear();
	point_lights.clear();
}
