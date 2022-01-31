#version 460 core

struct Material {
    vec4 diffuse;
    vec4 specular;
    vec4 reflective;
};

uniform sampler2D diffuse0;
uniform sampler2D specular0;
uniform sampler2D reflection0;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragColor;

out vec4 color;

uniform vec4 lightColor;
uniform float lightIntensity;
uniform vec3 lightPos;
uniform vec4 Ucolor;
uniform Material material;
uniform int noTex;

void main()
{
	float ambient_light = 0.2f;
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - FragPos);

	float diffuse = lightIntensity * max(dot(normal, lightDirection), 0.0f);

    vec4 diffMap;
    vec4 specMap;
    if (noTex == 1) {
        diffMap = material.diffuse;
        specMap = material.specular;
    }
    else {
        diffMap = texture(diffuse0, TexCoords);
        specMap = texture(specular0, TexCoords);
    }

    color = diffMap * lightColor * (diffuse + ambient_light);
}