#version 460 core
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 mvp;
} u_worldviewproj;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texture;
layout (location = 2) in vec4 a_mul_color;
layout (location = 3) in vec4 a_add_color;

layout (location = 0) out vec4 v_mul_color;
layout (location = 1) out vec4 v_add_color;
layout (location = 2) out vec2 v_texture;

void main()
{						
    v_mul_color = a_mul_color;
    v_add_color = a_add_color;
    v_texture = a_texture;
	gl_Position = u_worldviewproj.mvp * vec4(a_position, 1.0);
}

