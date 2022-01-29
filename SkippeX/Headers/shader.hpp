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
    void SetBool(const std::string& name, float value);
    void SetNoTexCoords(float value);


private:
    std::vector<shader> shaders; 
};