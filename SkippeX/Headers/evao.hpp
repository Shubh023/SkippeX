#pragma once
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>


struct Vertexx
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texUV;
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
    explicit VBO(std::vector<Vertexx>& vertices);

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