#pragma once
#include "render.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <stb/stb_truetype.h>
#include "types.hpp"
#include "platform.hpp"

namespace xs::render
{	
	struct image
	{
		xs::platform::texture_type	texture		= {};
		int							width		= -1;
		int							height		= -1;
		int							channels	= -1;
		std::size_t					string_id	= 0;
		std::string					file;
	};
	
	struct font_atlas
	{
		int								image_id		= -1;
		std::size_t						string_id		= 0;
		std::vector<stbtt_packedchar>	packed_chars	= {};
		stbtt_fontinfo					info			= {};		
		blob							buffer			= {};
		const unsigned char*			buffer_ptr		= nullptr;
		double							scale			= 0;
	};

    struct debug_vertex_format
    {
        glm::vec4 position;
        glm::vec4 color;
    };
	
	extern std::vector<image>				images;
	extern std::vector<font_atlas>			fonts;
	extern glm::vec2						offset;

	static int const                        lines_max = 16000;
    extern int                              lines_count;
    extern int                              lines_begin_count ;
    extern debug_vertex_format              lines_array[lines_max * 2];

    static int const                        triangles_max = 21800;
    extern int                              triangles_count;
    extern int                              triangles_begin_count;
    extern debug_vertex_format              triangles_array[triangles_max * 3];

    extern dbg_primitive                    current_primitive;
    extern glm::vec4                        current_color;

	inline glm::vec4 to_vec4(color c) { return glm::vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f); }
	inline void rotate_vector3d(glm::vec3& vec, float radians);
    inline void rotate_vector3d(glm::vec4& vec, float radians);
	inline void rotate_vector2d(glm::vec2& vec, float radians);
	void create_texture_with_data(xs::render::image& img, uchar* data);
}

inline void xs::render::rotate_vector3d(glm::vec3& vec, float radians)
{
	const float x = cos(radians) * vec.x - sin(radians) * vec.y;
	const float y = sin(radians) * vec.x + cos(radians) * vec.y;
	vec.x = x;
	vec.y = y;
}

inline void xs::render::rotate_vector3d(glm::vec4& vec, float radians)
{
    const float x = cos(radians) * vec.x - sin(radians) * vec.y;
    const float y = sin(radians) * vec.x + cos(radians) * vec.y;
    vec.x = x;
    vec.y = y;
}

inline void xs::render::rotate_vector2d(glm::vec2& vec, float radians)
{
	const float x = cos(radians) * vec.x - sin(radians) * vec.y;
	const float y = sin(radians) * vec.x + cos(radians) * vec.y;
	vec.x = x;
	vec.y = y;
}
