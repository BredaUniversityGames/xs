#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#import "shader_types.h"

struct vertex_to_fragment
{
    vec4 position [[position]];
    vec4 mul_color;
    vec4 add_color;
    vec2 texture;
};

vertex vertex_to_fragment vertex_shader(
    uint vertexID [[vertex_id]],
    constant sprite_vtx_format* vertices [[buffer(index_vertices)]],
    constant mat4* p_worldviewproj [[buffer(index_wvp)]])
{
    mat4 worldviewproj = mat4(*p_worldviewproj);
    
    vertex_to_fragment vtf;
    vtf.position = vertices[vertexID].position * worldviewproj;
    vtf.texture = vertices[vertexID].texture;
    vtf.add_color = vertices[vertexID].add_color;
    vtf.mul_color = vertices[vertexID].mul_color;
    
    return vtf;
}

fragment float4 fragment_shader(
    vertex_to_fragment vtf [[stage_in]],
    texture2d<ushort> sprite_texture [[texture(index_sprite_texture)]])
{
    constexpr sampler sprite_sampler (mag_filter::nearest,
                                      min_filter::nearest,
                                      address::clamp_to_edge,
                                      coord::normalized);
    // Sample the texture to obtain a color
    const ushort4 sprite_sample = sprite_texture.sample(sprite_sampler, vtf.texture);
    
    float4 f_sample = float4(sprite_sample);
    f_sample /= 255.0;
    //f_sample.a = pow(f_sample.a, 1.0 / 2.2);

    return float4(f_sample);
}
