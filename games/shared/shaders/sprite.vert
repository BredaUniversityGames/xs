#version 450 core

#extension GL_GOOGLE_include_directive : require

#include "uniforms.glsl"

struct instance_struct
{    
    mat4    wvp;        // 64
    vec4    mul_color;  // 16   
    vec4    add_color;  // 16
    uint    flags;      // 4
};

layout(std140, binding = INSTANCES_UBO_LOCATION) uniform TransformsUBO
{
    instance_struct instances[MAX_INSTANCES];
};

const uint c_flip_x = 32;
const uint c_flip_y = 64;   

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texture;

layout (location = 1) uniform mat4 u_worldviewproj;
layout (location = 2) uniform vec4 u_mul_color;
layout (location = 3) uniform vec4 u_add_color;
layout (location = 4) uniform uint u_flags;

out vec4 v_mul_color;
out vec4 v_add_color;
out vec2 v_texture;

void main()
{						
    v_mul_color = u_mul_color;
    v_add_color = u_add_color;
    v_texture = a_texture;
    if ((u_flags & c_flip_x) != 0)
        v_texture.x = 1.0 - v_texture.x;
    if ((u_flags & c_flip_y) != 0)
        v_texture.y = 1.0 - v_texture.y;
	gl_Position = u_worldviewproj * vec4(a_position, 0.0, 1.0);
}

