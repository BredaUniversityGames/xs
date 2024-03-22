#version 460 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_color;
out vec3 v_position;
out vec3 v_normal;
out vec2 v_texture_coordinate;
out vec4 v_color;
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec4 u_mul_color;

void main()
{
    v_normal = (u_model * vec4(a_normal, 0.0)).xyz;
    v_texture_coordinate = a_texture_coordinate;
    v_color = vec4(a_color, 1.0) * u_mul_color;
    v_position = (u_model * vec4(a_position, 1.0)).xyz;
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}