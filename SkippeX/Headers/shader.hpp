#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <cerrno>

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

private:
    std::vector<shader> shaders;
};