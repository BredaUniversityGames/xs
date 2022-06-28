#version 460 core
layout (location = 1) in vec3 a_position;
layout (location = 2) in vec2 a_texture;
layout (location = 3) in vec4 a_mul_color;
layout (location = 4) in vec4 a_add_color;
layout (location = 1) uniform mat4 u_worldviewproj;

out vec4 v_mul_color;
out vec4 v_add_color;
out vec2 v_texture;

void main()
{						
    v_mul_color = a_mul_color;
    v_add_color = a_add_color;
    v_texture = a_texture;
	gl_Position = u_worldviewproj * vec4(a_position, 1.0);
}

