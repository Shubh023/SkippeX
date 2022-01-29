#version 460 core
layout ( location = 0 ) in vec3 position;
layout ( location = 1 ) in vec3 normal;
layout ( location = 2 ) in vec2 texCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 projection_view;
uniform float noTexCoords;

void main( )
{
	gl_Position = projection_view * model * vec4( position, 1.0f );

	TexCoords = texCoords;
	FragPos = position;
	Normal = normal;
}