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
uniform float ambientStrength;
uniform float specularStrength;
uniform float fadeOff;
uniform vec3 lightPos;
uniform vec3 cameraPos;
uniform vec4 Ucolor;
uniform Material material;
uniform int noTex;
uniform float near;
uniform float far;


float linearizeDepth(float depth)
{
    return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
}

float logisticDepth(float depth)
{
    float steepness = 0.5f;
    float offset = 5.0f;
    float zVal = linearizeDepth(depth);
    return (1 / (1 + exp(-steepness * (zVal - offset))));
}

void main()
{

    float dist = (1.0f / fadeOff) * length(lightPos - FragPos);
    float a = 5.0;
    float b = 1.0;
    float depth = 1 - logisticDepth(gl_FragCoord.z);
    // float intensity = 1.0f / (a * dist * dist + b * dist + 1.0f);
    float intensity = 1 - depth;
    vec4 depthColor = lightColor * (1.0f - depth) + vec4(depth * vec3(0.85f, 0.85f, 0.90f), 1.0f);

    // Ambient
    vec4 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseStrength = max(dot(norm, lightDir), 0.0f);
    vec4 diffuse = diffuseStrength * lightColor;

    // Specular
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    vec4 specular = specularStrength * spec * lightColor;

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

    // Combine
    // color = depthColor * (diffMap * (ambient + diffuse) + specMap * (ambient + specular));
    color = depthColor;
}