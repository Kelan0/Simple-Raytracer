#version 430 core

in vec3 position;
in vec3 normal;
in vec2 texture;
in vec4 colour;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 p_texture;

void main(void)
{
	p_texture = position.xy * 0.5 + vec2(0.5, 0.5);
	
	gl_Position = vec4(position, 1.0);
	
	//gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}