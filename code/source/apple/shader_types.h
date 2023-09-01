//
//  Header containing types and enum constants shared between Metal shaders and Swift/ObjC source
//

/*
#ifndef shader_types_h
#define shader_types_h

#ifdef __METAL_VERSION__
#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
typedef metal::int32_t EnumBackingType;
#else
#import <Foundation/Foundation.h>
typedef NSInteger EnumBackingType;
#endif

#include <simd/simd.h>

typedef NS_ENUM(EnumBackingType, BufferIndex)
{
    BufferIndexMeshPositions = 0,
    BufferIndexMeshGenerics  = 1,
    BufferIndexUniforms      = 2
};

typedef NS_ENUM(EnumBackingType, VertexAttribute)
{
    VertexAttributePosition  = 0,
    VertexAttributeTexcoord  = 1,
};

typedef NS_ENUM(EnumBackingType, TextureIndex)
{
    TextureIndexColor    = 0,
};

typedef struct
{
    matrix_float4x4 projectionMatrix;
    matrix_float4x4 modelViewMatrix;
} Uniforms;

#endif
*/

#pragma once

#ifdef __METAL_VERSION__

#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type
typedef metal::int32_t EnumBackingType;
typedef vector_float2 vec2;
typedef vector_float3 vec3;
typedef vector_float4 vec4;
typedef matrix_float4x4 mat4;

#else

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

typedef enum input_index
{
    index_vertices  = 0,
    index_wvp       = 1,
} input_index;

typedef enum input_texture_index
{
    index_sprite_texture  = 0,
} input_texture_index;


// Buffer index values shared between shader and C code to ensure Metal shader buffer inputs
// match Metal API buffer set calls.
typedef enum AAPLVertexInputIndex
{
    AAPLVertexInputIndexVertices     = 0,
    AAPLVertexInputIndexViewportSize = 1,
} AAPLVertexInputIndex;

typedef struct
{
    vector_float2 position;
    vector_float4 color;
} AAPLVertex;

