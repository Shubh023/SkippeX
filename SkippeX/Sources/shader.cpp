#include "shader.hpp"
#include <iostream>

std::string readFile(const char* filename) {
    std::ifstream in(filename, std::ios::binary);
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return { contents };
    }
    throw(errno);
}

Shader::Shader(GLenum _type, const char* _file) :
    file(_file), type(_type) 
{}

Shader Shader::Compile() {

    // Read and store Shader Files to List of characters
    std::string shaderCode = readFile(file);
    const char* shaderSource = shaderCode.c_str();

    // Setup Vertex Shader and Compile it
    GLuint shaderID = glCreateShader(type);
    glShaderSource(shaderID, 1, &shaderSource, NULL);
    glCompileShader(shaderID);


    int status;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        int length;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(shaderID, length, &length, message);
        std::cout << "Failed to compile shader : " << type << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(shaderID);
    }
    // Create Shader program and get store its reference in ID
    ID = glCreateProgram();

    // Attach the Vertex and Fragment Shaders to the Shader Program
    glAttachShader(ID, shaderID);
    
    // Wrap-Up | Link all shaders together into the shader program
    glLinkProgram(ID);
    
    // Delete shader
    glDeleteShader(shaderID);
    return *this;
}

void Shader::Activate() {
    glUseProgram(ID);
}

void Shader::Delete() {
    glDeleteProgram(ID);
}



LinkedShader::LinkedShader(std::vector<shader> _shaders) : 
    shaders(_shaders)
{}

LinkedShader LinkedShader::Compile() {


    std::vector<GLuint> shader_ids(shaders.size());

    for (auto s : shaders) {

        auto type = s.first;
        auto file = s.second;
        // Read and store Shader Files to List of characters
        std::string shaderCode = readFile(file);
        const char* shaderSource = shaderCode.c_str();

        // Setup Vertex Shader and Compile it
        GLuint shaderID = glCreateShader(type);
        glShaderSource(shaderID, 1, &shaderSource, NULL);
        glCompileShader(shaderID);


        int status;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            int length;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &length);
            char* message = (char*)alloca(length * sizeof(char));
            glGetShaderInfoLog(shaderID, length, &length, message);
            std::cout << "Failed to compile shader : " << type << std::endl;
            std::cout << message << std::endl;
            glDeleteShader(shaderID);
        }
        else
        {
            shader_ids.push_back(shaderID);
        }
    }
    // Create Shader program and get store its reference in ID
    ID = glCreateProgram();

    // Attach the Vertex and Fragment Shaders to the Shader Program
    for (auto sid : shader_ids) {
        glAttachShader(ID, sid);
    }
    // Wrap-Up | Link all shaders together into the shader program
    glLinkProgram(ID);

    // Delete shader
    for (auto sid : shader_ids) {
        glDeleteShader(sid);
    }
    return *this;
}

void LinkedShader::Activate() {
    glUseProgram(ID);
}

void LinkedShader::Delete() {
    glDeleteProgram(ID);
}

void LinkedShader::SetMat4(const std::string& name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void LinkedShader::SetVec3(const std::string& name, glm::vec3 value)
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}