#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <cerrno>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

typedef std::pair<GLenum, const char*> shader;

std::string readFile(const char* filename);

class Shader {
public:
    GLuint ID;
    Shader(GLenum type, const char* file);

    Shader Compile();
    void Activate();
    void Delete();
    
private :
    const char* file;
    GLenum type;
};


class LinkedShader {
public:
    GLuint ID;
    LinkedShader(std::vector<shader> shaders);
    
    LinkedShader Compile();
    void Activate();
    void Delete();
    void SetMat4(const std::string& name, glm::mat4 value);
    void SetVec3(const std::string& name, glm::vec3 value);
    void SetVec4(const std::string& name, glm::vec4 value);
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);


private:
    std::vector<shader> shaders; 
};


class MLine {
    int shaderProgram;
    unsigned int VBO, VAO;

    glm::mat4 MVP = glm::mat4(1.0);
    glm::vec3 lineColor;
public:
    std::vector<float> vertices;

    MLine(std::vector<float> _vertices) {
        vertices = _vertices;
    };
    void setup() {
        lineColor = glm::vec3(1,1,1);

        const char *vertexShaderSource = "#version 330 core\n"
                                         "layout (location = 0) in vec3 aPos;\n"
                                         "uniform mat4 MVP;\n"
                                         "void main()\n"
                                         "{\n"
                                         "   gl_Position = MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                         "}\0";
        const char *fragmentShaderSource = "#version 330 core\n"
                                           "out vec4 FragColor;\n"
                                           "uniform vec3 color;\n"
                                           "void main()\n"
                                           "{\n"
                                           "   FragColor = vec4(color, 1.0f);\n"
                                           "}\n\0";

        // vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors

        // fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors

        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices)*vertices.size(), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void setMVP(glm::mat4 mvp) {
        MVP = mvp;
    }

    void setColor(glm::vec3 color) {
        lineColor = color;
    }

    int draw() {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &lineColor[0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, vertices.size() / 3);
        return 0;
    }

    ~MLine() {

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};