#version 330 core

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

out vec4 color;

uniform sampler2D texture_diffuse;
uniform vec4 lightColor;
uniform float lightIntensity;
uniform vec3 lightPos;

void main( )
{

	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - FragPos);

	float diffuse = lightIntensity * max(dot(normal, lightDirection), 0.0f);

	color = normalize(texture( texture_diffuse, TexCoords)) * normalize(lightColor) * diffuse;
}