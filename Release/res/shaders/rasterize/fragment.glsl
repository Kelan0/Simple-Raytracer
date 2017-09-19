#version 430 core

in vec3 p_position;
in vec3 p_normal;
in vec2 p_texture;
in vec4 p_colour;

out vec4 out_colour;

void main(void)
{
	out_colour = vec4(p_colour);
}