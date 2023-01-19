#version 460 core
layout (binding = 0) uniform sampler2D s_image;

layout (location = 0) in vec4 v_mul_color;
layout (location = 1) in vec4 v_add_color;
layout (location = 2) in vec2 v_texture;
layout (location = 0) out vec4 frag_color;

void main()
{
    vec4 color = texture(s_image, v_texture) * v_mul_color + v_add_color;
    frag_color = color;
}