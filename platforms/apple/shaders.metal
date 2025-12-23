#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#import "shader_types.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Sprite rendering - Mesh-based (new approach)
////////////////////////////////////////////////////////////////////////////////////////////////////

struct vertex_to_fragment
{
    vec4 position [[position]];
    vec4 mul_color;
    vec4 add_color;
    vec2 texture;
};

vertex vertex_to_fragment vertex_shader_mesh(
    uint vertexID [[vertex_id]],
    constant vec2* positions [[buffer(index_vertices)]],
    constant vec2* texcoords [[buffer(index_texcoords)]],
    constant mat4* p_worldviewproj [[buffer(index_wvp)]],
    constant sprite_instance_data* instance [[buffer(index_instance)]])
{
    mat4 worldviewproj = mat4(*p_worldviewproj);
    sprite_instance_data inst = *instance;

    // Get vertex position and texcoord from mesh buffers
    vec2 pos = positions[vertexID];
    vec2 uv = texcoords[vertexID];

    // Scale position by sprite bounds
    float xs = inst.xy.z - inst.xy.x;  // Width
    float ys = inst.xy.w - inst.xy.y;  // Height
    vec2 scaled_pos = vec2(pos.x * xs, pos.y * ys);

    // Apply anchor offset based on flags
    vec2 anchor = vec2(0.0, 0.0);
    if ((inst.flags & (1u << 3)) != 0) // center_x
        anchor.x = xs * 0.5;
    if ((inst.flags & (1u << 4)) != 0) // center_y
        anchor.y = ys * 0.5;
    else if ((inst.flags & (1u << 2)) != 0) // top
        anchor.y = ys;

    scaled_pos -= anchor;

    // Apply rotation
    if (inst.rotation != 0.0) {
        float c = cos(inst.rotation);
        float s = sin(inst.rotation);
        vec2 rotated = vec2(
            scaled_pos.x * c - scaled_pos.y * s,
            scaled_pos.x * s + scaled_pos.y * c
        );
        scaled_pos = rotated;
    }

    // Apply scale
    scaled_pos *= inst.scale;

    // Apply world position
    scaled_pos += inst.position;

    // Transform to clip space
    vertex_to_fragment vtf;
    vtf.position = vec4(scaled_pos, 0.0, 1.0) * worldviewproj;

    // Handle texture coordinate flipping
    vec2 final_uv = uv;
    if ((inst.flags & (1u << 5)) != 0) { // flip_x
        final_uv.x = inst.uv.x + inst.uv.z - uv.x;
    }
    if ((inst.flags & (1u << 6)) != 0) { // flip_y
        final_uv.y = inst.uv.y + inst.uv.w - uv.y;
    }

    vtf.texture = final_uv;
    vtf.add_color = inst.add_color;
    vtf.mul_color = inst.mul_color;

    return vtf;
}

// Legacy inline vertex shader (keep for debug geometry)
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
    constant vec2* p_resolution [[buffer(index_resolution)]],
    constant screen_vtx_format* vertices [[buffer(index_vertices)]])
{
    vec2 res = vec2(*p_resolution);
    screen_to_fragment stf;
    stf.position = float4(0.0, 0.0, 0.0, 1.0);
    stf.position.x = (vertices[vertexID].position.x / res.x) - 1.0;
    stf.position.y = (vertices[vertexID].position.y / res.y) - 1.0;
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
