//  Header containing types and enum constants shared between Metal shaders and ObjC++ source

#pragma once

#ifdef __METAL_VERSION__    // Metal

#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
typedef metal::int32_t EnumBackingType;
typedef vector_float2 vec2;
typedef vector_float3 vec3;
typedef vector_float4 vec4;
typedef matrix_float4x4 mat4;

#else                       // C++

#include <glm/glm.hpp>
#import <Foundation/Foundation.h>
typedef NSInteger EnumBackingType;
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

#endif

typedef struct
{
    vec4 add_color;
    vec4 mul_color;
    vec4 position;
    vec2 texture;
    vec2 padding;
} sprite_vtx_format;

typedef struct
{
    vec2 position;
    vec2 texcoord;
} screen_vtx_format;

typedef enum input_index
{
    index_vertices = 0,
    index_wvp = 1,
    index_resolution = 2
} input_index;

typedef enum input_texture_index
{
    index_sprite_texture  = 0,
} input_texture_index;

typedef struct
{
    vec4 position;
    vec4 color;    
} debug_vtx_format;
