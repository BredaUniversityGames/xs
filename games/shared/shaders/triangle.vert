#version 460 core
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 mvp;
} u_worldviewproj;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

layout (location = 0) out vec4 v_color;

void main()
{						
    v_color = a_color;
	gl_Position = u_worldviewproj.mvp * vec4(a_position, 1.0);
}

