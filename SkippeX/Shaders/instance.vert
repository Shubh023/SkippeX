#version 460 core
layout ( location = 0 ) in vec3 aPos;
layout ( location = 1 ) in vec3 aNormal;
layout ( location = 2 ) in vec4 aColor;
layout ( location = 3 ) in vec2 aTexCoord;
layout ( location = 4 ) in mat4 instanceMatrix;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out vec4 FragColor;

uniform vec3 cameraPos;
uniform mat4 view;
uniform mat4 projection;
uniform float noTexCoords;
uniform int noTex;
uniform int noShading;

void main( )
{
	gl_Position = projection * view * instanceMatrix * vec4( aPos, 1.0f );

	TexCoords = aTexCoord;
	FragPos = vec3(instanceMatrix * vec4(aPos, 1.0)); // Vertex in world space
	FragColor = aColor;
	if (noShading == 1)
		Normal = vec3(1.0f);
	else
		Normal = inverse(transpose(mat3(instanceMatrix))) * aNormal; // Normal in world space
}