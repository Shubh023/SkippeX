#version 460 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragColor;

out vec4 color;

uniform vec4 lightColor;

void main( )
{
	color = lightColor * FragColor;
}