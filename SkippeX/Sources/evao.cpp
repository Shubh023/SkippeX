#include "evao.hpp"


// EBO
EBO::EBO(std::vector<GLuint>& indices) {
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);
}

void EBO::bind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void EBO::unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::del() {
    glDeleteBuffers(1, &ID);
}


// VBO
VBO::VBO(std::vector<Vertex>& vertices) {
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VBO::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

void VBO::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::del() {
    glDeleteBuffers(1, &id);
}



// VAO
VAO::VAO() {
    glGenVertexArrays(1, &ID);
}

void VAO::link(VBO& VBO, GLuint format, GLuint np, GLenum type, GLsizeiptr sd, void* off) {
    VBO.bind();
    glVertexAttribPointer(format, np, type, GL_FALSE, sd, off);
    glEnableVertexAttribArray(format);
    VBO.unbind();
}

void VAO::bind() {
    glBindVertexArray(ID);
}

void VAO::unbind() {
    glBindVertexArray(0);
}

void VAO::del() {
    glDeleteVertexArrays(1, &ID);
}