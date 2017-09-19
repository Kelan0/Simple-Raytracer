#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture;
layout (location = 3) in vec4 colour;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 p_texture;
out vec4 p_colour;

void main(void)
{
	p_texture = texture;
	p_colour = colour;
	
	gl_Position = projectionMatrix * vec4(position, 1.0);
}