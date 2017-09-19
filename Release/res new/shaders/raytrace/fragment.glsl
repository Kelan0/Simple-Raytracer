#version 430 core

in vec2 p_texture;
in vec4 p_colour;

uniform sampler2D sampler;
uniform bool font;

out vec4 out_colour;

void main(void)
{
	vec4 texColour = texture2D(sampler, p_texture);
	
	if (font)
	{
		out_colour = p_colour * texColour.r;
	} else
	{
		out_colour = p_colour * texColour;
	}
}