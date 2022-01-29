#version 330 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 color;

uniform sampler2D texture_diffuse;
uniform vec4 lightColor;
uniform vec4 lightIntensity;

void main( )
{
	color = normalize(lightColor * lightIntensity);
}