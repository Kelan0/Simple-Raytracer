#version 430 core

in vec2 p_texture;

uniform sampler2D sampler;

out vec4 out_colour;

void main(void)
{
	//out_colour = vec4(1.0);
	out_colour = texture2D(sampler, p_texture);
}