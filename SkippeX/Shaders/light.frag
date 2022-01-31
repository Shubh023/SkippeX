#version 460 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragColor;

out vec4 color;

uniform sampler2D diffuse0;
uniform sampler2D specular0;
uniform sampler2D reflection0;
uniform vec4 lightColor;
uniform float lightIntensity;

void main( )
{
	color = lightColor * FragColor;
}