#version 450 core
#extension GL_GOOGLE_include_directive : require
#include "uniforms.glsl"

const uint c_flip_x = 32;
const uint c_flip_y = 64;   

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texture;

out vec4 v_mul_color;
out vec4 v_add_color;
out vec2 v_texture;


void main()
{			
    mat4 wvp = instances[gl_InstanceID].wvp;
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

