#version 460 core

precision mediump float;

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

uniform vec3 mousePos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float near;
uniform float far;
uniform mat4 invProjectionView;
uniform vec3 windowDimensions;

const int marchSteps = 64;
const float epsilon = 1.0e-5;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

float raymarch(vec3 rayOrigin, vec3 rayDirection, float near, float far)
{
    float dist = near + epsilon;

    float t = 0.0;

    for (int i = 0; i < marchSteps; i++)
    {
        if (abs(dist) < near || t > far)
        break;

        t += dist;
        vec3 p = rayOrigin + t * rayDirection;
    }

    return t;
}

void main()
{
    float width = windowDimensions.x;
    float height = windowDimensions.y;
    float mouseX = mousePos.x / (width  * 0.5f) - 1.0f;
    float mouseY = mousePos.y / (height * 0.5f) - 1.0f;

    vec4 screenPos = vec4(mouseX, -mouseY, 1.0f, 1.0f);
    vec4 worldPos = inverse(projection * view) * screenPos;
    vec3 dir = normalize(vec3(worldPos));

    Ray ray;
    ray.origin = cameraPos;
    ray.direction = dir;

    float t = raymarch(ray.origin, ray.direction, near, far);
    vec4 tColor = vec4(t);
    vec4 lColor = lightColor;

    float dist = (1.0f / fadeOff) * length(lightPos - FragPos);
    float a = 5.0;
    float b = 1.0;
    float intensity = t * 1.0f / (a * dist * dist + b * dist + 1.0f);

    // Ambient
    vec4 ambient = ambientStrength * lColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseStrength = max(dot(norm, lightDir), 0.0f);
    vec4 diffuse = diffuseStrength * lColor;

    // Specular

    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    vec4 specular = specularStrength * spec * lColor;

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
    color = intensity * (diffMap * (ambient + diffuse) + specMap * (ambient + specular));
}