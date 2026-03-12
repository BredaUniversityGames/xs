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

    // Shapes are rendered with less transformations
    // Vertex positions are already in world coordinates
    const uint is_shape = 1 << 8;  // sprite_flags::is_shape
    if (inst.flags == is_shape) {
        // Apply scale and rotation matrix
        float c = cos(inst.rotation);
        float s = sin(inst.rotation);
        vec2 scaled_pos = vec2(
            pos.x * inst.scale.x * c - pos.y * inst.scale.y * s,
            pos.x * inst.scale.x * s + pos.y * inst.scale.y * c
        );
        scaled_pos += inst.position;

        vertex_to_fragment vtf;
        vtf.position = vec4(scaled_pos, 0.0, 1.0) * worldviewproj;
        vtf.texture = uv;
        vtf.add_color = inst.add_color;
        vtf.mul_color = inst.mul_color;
        return vtf;
    }

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// CRT Postprocess - compute kernel
////////////////////////////////////////////////////////////////////////////////////////////////////
kernel void kernel_postprocess(
    texture2d<float, access::sample> input_texture  [[texture(pp_index_input)]],
    texture2d<float, access::write>  output_texture [[texture(pp_index_output)]],
    constant postprocess_uniforms&   uniforms        [[buffer(pp_index_uniforms)]],
    uint2 thread_pos [[thread_position_in_grid]])
{
    uint2 out_size = uint2((uint)uniforms.output_size.x, (uint)uniforms.output_size.y);
    if (thread_pos.x >= out_size.x || thread_pos.y >= out_size.y)
        return;

    float2 uv = (float2(thread_pos) + 0.5) / uniforms.output_size;
    float2 pos = uv * 2.0 - 1.0;
    float vignette = 1.0;

    // Barrel warp distortion
    if (uniforms.enable_warp) {
        pos *= float2(
            1.0 + (pos.y * pos.y) * uniforms.warp_x,
            1.0 + (pos.x * pos.x) * uniforms.warp_y);
    }

    // Vignette
    if (uniforms.enable_vignette) {
        vignette = 1.0 - (pos.x * pos.x + pos.y * pos.y) * 0.2;
        vignette = clamp(vignette, 0.0, 1.0);
    }

    uv = pos * 0.5 + 0.5;

    float3 color = float3(0.0);
    if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0) {
        constexpr sampler tex_sampler(coord::normalized, address::clamp_to_edge, filter::linear);

        // Chromatic aberration
        if (uniforms.enable_chromatic) {
            float2 offset = float2(uniforms.chromatic_offset) / uniforms.output_size;
            float2 uv_r = clamp(uv - float2(offset.x, 0.0), 0.0, 1.0);
            float2 uv_b = clamp(uv + float2(offset.x, 0.0), 0.0, 1.0);
            float r = input_texture.sample(tex_sampler, uv_r).r;
            float g = input_texture.sample(tex_sampler, uv).g;
            float b = input_texture.sample(tex_sampler, uv_b).b;
            color = float3(r, g, b);
        } else {
            color = input_texture.sample(tex_sampler, uv).rgb;
        }

        color *= vignette;

        // Scanlines
        if (uniforms.enable_scanlines) {
            float scanline_y = uv.y * uniforms.input_size.y;
            float scanline = sin(scanline_y * M_PI_F * uniforms.scanline_thickness) * 0.5 + 0.5;
            scanline = 1.0 - (scanline * uniforms.scanline_strength);
            color *= scanline;
        }

        // Phosphor mask (aperture grille)
        if (uniforms.enable_phosphor) {
            float3 mask = float3(1.0);
            float x = fmod(uv.x * uniforms.input_size.x, 3.0);
            if (x < 1.0)
                mask.r = 1.0 - uniforms.phosphor_strength;
            else if (x < 2.0)
                mask.g = 1.0 - uniforms.phosphor_strength;
            else
                mask.b = 1.0 - uniforms.phosphor_strength;
            color *= mask;
        }

        color *= uniforms.brightness_boost;
    }

    output_texture.write(float4(color, 1.0), thread_pos);
}
