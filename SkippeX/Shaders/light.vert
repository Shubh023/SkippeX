#version 460 core
layout ( location = 0 ) in vec3 aPos;
layout ( location = 1 ) in vec3 aNormal;
layout ( location = 2 ) in vec4 aColor;
layout ( location = 3 ) in vec2 aTexCoord;

out vec2 TexCoords;
out vec3 Normal;
out vec4 FragColor;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main( )
{
	gl_Position = projection * view * model * vec4( aPos, 1.0f );
	TexCoords = aTexCoord;
	FragPos = aPos;
	FragColor = aColor;
	Normal = aNormal;
}