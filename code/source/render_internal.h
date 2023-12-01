#pragma once

#include "render.h"
#include <glm/glm.hpp>
#include <vector>
#include <stb/stb_truetype.h>


#if defined(PLATFORM_PS5)
	#include <agc.h>	
	using textureType = sce::Agc::Core::Texture;
#elif defined(PLATFORM_PC) || defined(PLATFORM_SWITCH)
    using textureType = unsigned int;
#elif defined(__APPLE__)
    #import <Foundation/Foundation.h>
    #import <MetalKit/MetalKit.h>
    typedef id<MTLTexture> textureType;
#endif

class ImDrawData;

namespace xs::render::internal
{	
	struct image
	{
		textureType	texture		= {};
		int			width		= -1;
		int			height		= -1;
		int			channels	= -1;
		std::size_t	string_id	= 0;
		std::string file;
	};

	struct sprite
	{
		int image_id	= -1;
		glm::vec2 from	= {};
		glm::vec2 to	= {};		
	};

	struct sprite_queue_entry
	{
		int sprite_id	= -1;
		double			x = 0.0;
		double			y = 0.0;
		double			z = 0.0;
		double			scale = 1.0;
		double			rotation = 0.0;
		color			mul_color = {};
		color			add_color = {};
		unsigned int	flags = 0;
	};

	struct font_atlas
	{
		int								image_id		= -1;
		std::size_t						string_id		= 0;
		std::vector<stbtt_packedchar>	packed_chars	= {};
		stbtt_fontinfo					info			= {};
		unsigned char*					buffer			= nullptr;
		double							scale			= 0;
	};

	extern std::vector<sprite_queue_entry>	sprite_queue;
	extern std::vector<sprite>				sprites;
	extern std::vector<font_atlas>			fonts;
	extern std::vector<image>				images;
	extern glm::vec2						offset;

	inline glm::vec4 to_vec4(color c) { return glm::vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f); }
	inline void rotate_vector3d(glm::vec3& vec, float radians);
    inline void rotate_vector3d(glm::vec4& vec, float radians);
	inline void rotate_vector2d(glm::vec2& vec, float radians);
	void create_texture_with_data(xs::render::internal::image& img, uchar* data);
}

inline void xs::render::internal::rotate_vector3d(glm::vec3& vec, float radians)
{
	const float x = cos(radians) * vec.x - sin(radians) * vec.y;
	const float y = sin(radians) * vec.x + cos(radians) * vec.y;
	vec.x = x;
	vec.y = y;
}

inline void xs::render::internal::rotate_vector3d(glm::vec4& vec, float radians)
{
    const float x = cos(radians) * vec.x - sin(radians) * vec.y;
    const float y = sin(radians) * vec.x + cos(radians) * vec.y;
    vec.x = x;
    vec.y = y;
}

inline void xs::render::internal::rotate_vector2d(glm::vec2& vec, float radians)
{
	const float x = cos(radians) * vec.x - sin(radians) * vec.y;
	const float y = sin(radians) * vec.x + cos(radians) * vec.y;
	vec.x = x;
	vec.y = y;
}
