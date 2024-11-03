#version 450 core
#extension GL_GOOGLE_include_directive : require
#include "uniforms.glsl"

const uint c_flip_x = 32;
const uint c_flip_y = 64;   

layout (location = 0) uniform mat4 u_view_proj;

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texture;

out vec4 v_mul_color;
out vec4 v_add_color;
out vec2 v_texture;

void main()
{	
    vec2 pos = instances[gl_InstanceID].position;
    vec2 scale = instances[gl_InstanceID].scale;
    float rotation = instances[gl_InstanceID].rotation;    
    mat4 wvp = mat4(1.0);
    wvp[0][0] = scale.x * cos(rotation);
    wvp[0][1] = scale.x * sin(rotation);
    wvp[1][0] = -scale.y * sin(rotation);
    wvp[1][1] = scale.y * cos(rotation);
    wvp[3][0] = pos.x;
    wvp[3][1] = pos.y;
    wvp = u_view_proj * wvp;

    uint flags = instances[gl_InstanceID].flags;
    v_mul_color = instances[gl_InstanceID].mul_color;
    v_add_color = instances[gl_InstanceID].add_color;
    v_texture = a_texture;
    if ((flags & c_flip_x) != 0)
        v_texture.x = 1.0 - v_texture.x;
    if ((flags & c_flip_y) != 0)
        v_texture.y = 1.0 - v_texture.y;
    gl_Position = wvp * vec4(a_position, 0.0, 1.0);
}

