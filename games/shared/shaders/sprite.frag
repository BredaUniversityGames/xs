#version 460 core
layout (binding = 0) uniform sampler2D s_image;

in vec4 v_mul_color;
in vec4 v_add_color;
in vec2 v_texture;
out vec4 frag_color;

void main()
{
    vec4 color = texture(s_image, v_texture) * v_mul_color + v_add_color;
    frag_color = color;
}