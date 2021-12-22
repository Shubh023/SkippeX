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
    
    void SetUniform(const std::string& name);

private:
    std::vector<shader> shaders; 
};