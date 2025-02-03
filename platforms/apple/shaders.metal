#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#import "shader_types.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Sprite rendering
////////////////////////////////////////////////////////////////////////////////////////////////////

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
    f_sample *= vtf.mul_color;
    f_sample += vtf.add_color;
    
    //f_sample.a = pow(f_sample.a, 1.0 / 2.2);

    return float4(f_sample);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug rendering
////////////////////////////////////////////////////////////////////////////////////////////////////
struct debug_to_fragment
{
    vec4 position [[position]];
    vec4 color;
};

vertex debug_to_fragment vertex_shader_debug(
   uint vertexID [[vertex_id]],
   constant debug_vtx_format* vertices [[buffer(index_vertices)]],
   constant mat4* p_worldviewproj [[buffer(index_wvp)]])
{
    mat4 worldviewproj = mat4(*p_worldviewproj);
    debug_to_fragment dtf;
    //vec4 h_position = , 1.0);
    dtf.position = vertices[vertexID].position * worldviewproj;
    dtf.color = vertices[vertexID].color;
    return dtf;
}

fragment float4 fragment_shader_debug(debug_to_fragment dtf [[stage_in]])
{
    return dtf.color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Render to screen
////////////////////////////////////////////////////////////////////////////////////////////////////
struct screen_to_fragment
{
    vec4 position [[position]];
    vec2 texture;
};

vertex screen_to_fragment vertex_shader_screen(
    uint vertexID [[vertex_id]],
    constant screen_vtx_format* vertices [[buffer(index_vertices)]])
{
    screen_to_fragment stf;
    stf.position = float4(0.0, 0.0, 0.0, 1.0);
    stf.position.x = vertices[vertexID].position.x;
    stf.position.y = vertices[vertexID].position.y;
    stf.texture = vertices[vertexID].texcoord;
    return stf;
}

fragment float4 fragment_shader_screen(
    screen_to_fragment stf [[stage_in]],
    texture2d<float> sprite_texture [[texture(index_sprite_texture)]])
{
    constexpr sampler sprite_sampler (mag_filter::nearest,
                                      min_filter::nearest,
                                      address::clamp_to_edge,
                                      coord::normalized);
    const float4 sprite_sample = sprite_texture.sample(sprite_sampler, stf.texture);
    return sprite_sample;
}
