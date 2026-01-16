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

// Mesh-based sprite rendering structures
typedef struct
{
    vec4 xy;            // Sprite bounds (from_x, from_y, to_x, to_y)
    vec4 uv;            // Texture coords (u0, v0, u1, v1)
    vec4 mul_color;     // Multiply color
    vec4 add_color;     // Additive color
    vec2 position;      // World position
    vec2 scale;         // Scale factor
    float rotation;     // Rotation angle
    unsigned int flags; // Sprite flags
    float padding[2];   // Padding to align to 16-byte boundary (96 bytes total)
} sprite_instance_data;

typedef enum input_index
{
    index_vertices = 0,
    index_texcoords = 1,
    index_wvp = 2,
    index_instance = 3,
    index_resolution = 4
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
