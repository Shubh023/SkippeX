#pragma once
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>


struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec2 TexCoords;
};

class EBO
{
public:
    explicit EBO(std::vector<GLuint>& indices);

    void bind();
    void unbind();
    void del();

    GLuint ID;
};


class VBO {
public:
    explicit VBO(std::vector<Vertex>& vertices);
    VBO(std::vector<glm::mat4>& mat4s);
    void bind() const;
    static void unbind();
    void del();

    GLuint id;
};

class VAO {
public:
    VAO();

    static void link(VBO& VBO, GLuint format, GLuint np, GLenum type, GLsizeiptr sd, void* off);
    void bind();
    void unbind();
    void del();

    GLuint ID{};
};