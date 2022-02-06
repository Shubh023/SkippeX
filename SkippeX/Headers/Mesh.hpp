#pragma once

#include "shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include "evao.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;




class Texture {
public:
    Texture() {};
    Texture(std::string dir, std::string path, aiTextureType type)
        : dir(dir), path(path), type(type)
    {};


    void load(bool flip) {
        string filename = string(path);
        filename = dir + '/' + filename;
        GLuint textureID;
        glGenTextures(1, &id);

        int width;
        int height;
        int channels;
        // unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb);
        unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb);

        GLenum colorMode = GL_RGB;

        if (channels == 1)
            colorMode == GL_RED;
        if (channels == 4)
            colorMode == GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, colorMode, width, height, 0, colorMode, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        // std::cout << width << " " <<  height << " " << channels << std::endl;
        stbi_image_free(image);
    }

    void bind() {
        glBindTexture(GL_TEXTURE_2D, id);
    }

    unsigned int id;
    aiTextureType type;
    std::string dir;
    std::string path;
};

class Mesh
{
public:
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<Texture> textures;
    aiColor4D diffuse;
    aiColor4D specular;
    aiColor4D reflective;
    VAO mVAO;

    unsigned int instancing;

    bool noTex = false;

    Mesh(vector<Vertex> vertices, vector<GLuint> indices, vector<Texture> textures, unsigned int instancing = 1, std::vector<glm::mat4> instancesMatrix = {})
    : vertices(vertices), indices(indices), textures(textures), noTex(false), instancing(instancing), mVBO(VBO(vertices)), mEBO(EBO(indices))
    {
        this->Setup(instancesMatrix);
    }

    Mesh(vector<Vertex> vertices, vector<GLuint> indices, aiColor4D diffuse, aiColor4D specular, aiColor4D reflective, unsigned int instancing = 1, std::vector<glm::mat4> instancesMatrix = {})
            :
            vertices(vertices),
            indices(indices),
            noTex(true),
            diffuse(diffuse),
            specular(specular),
            reflective(reflective),
            instancing(instancing),
            mVBO(VBO(vertices)),
            mEBO(EBO(indices))
    {
        this->Setup(instancesMatrix);
    }

    void Draw(LinkedShader shader)
    {

        if (noTex)
        {
            shader.SetVec4("material.diffuse", glm::vec4(diffuse[0], diffuse[1], diffuse[2],diffuse[3]));
            shader.SetVec4("material.specular", glm::vec4(specular[0], specular[1], specular[2],specular[3]));
            shader.SetVec4("material.reflective", glm::vec4(reflective[0], reflective[1], reflective[2],reflective[3]));
            shader.SetInt("noTex", 1);
        }
        else {
            GLuint diffuse = 0;
            GLuint specular = 0;
            GLuint reflection = 0;

            for (GLuint i = 0; i < this->textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i);

                std::string name = "diffuse0";
                switch (textures[i].type) {
                    case aiTextureType_DIFFUSE:
                        name = "diffuse" + std::to_string(diffuse++);
                        break;
                    case aiTextureType_SPECULAR:
                        name = "specular" + std::to_string(specular++);
                        break;
                    case aiTextureType_REFLECTION:
                        name = "reflection" + std::to_string(reflection++);
                        break;
                }
                glUniform1i(glGetUniformLocation(shader.ID, name.data()), i);
                glActiveTexture(GL_TEXTURE0 + i);
                // glBindTexture(GL_TEXTURE_2D, textures[i].id);
                textures[i].bind();
            }
        }
        glBindVertexArray(mVAO.ID);
        if (instancing == 1)
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        else
            glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instancing);
        glBindVertexArray(0);

        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        // glActiveTexture(GL_TEXTURE0);
    }

    void Delete() {
        mVBO.del();
        mEBO.del();
        mVAO.del();
    }

private:
    VBO mVBO;
    EBO mEBO;

    void Setup(std::vector<glm::mat4> instancesMatrix){

        // Setup VAO VBO EBO
        glGenVertexArrays(1, &mVAO.ID);
        glGenBuffers(1, &mVBO.id);
        glGenBuffers(1, &mEBO.ID);

        glBindVertexArray(mVAO.ID);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO.id);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->mEBO.ID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

        // Vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid * )0);

        // Vertex Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid * ) offsetof(Vertex, Normal));

        // Vertex Colors
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid * ) offsetof(Vertex, Color));

        // Texture Coordinates
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid * ) offsetof(Vertex, TexCoords));

        VBO instanceVBO(instancesMatrix);

        if (instancing != 1) {
            instanceVBO.bind();

            mVAO.link(instanceVBO, 4, 4, GL_FLOAT, sizeof(glm::mat4), (void *)0);
            mVAO.link(instanceVBO, 5, 4, GL_FLOAT, sizeof(glm::mat4), (void *)(1 * sizeof(glm::vec4)));
            mVAO.link(instanceVBO, 6, 4, GL_FLOAT, sizeof(glm::mat4), (void *)(2 * sizeof(glm::vec4)));
            mVAO.link(instanceVBO, 7, 4, GL_FLOAT, sizeof(glm::mat4), (void *)(3 * sizeof(glm::vec4)));

            glVertexAttribDivisor(4, 1);
            glVertexAttribDivisor(5, 1);
            glVertexAttribDivisor(6, 1);
            glVertexAttribDivisor(7, 1);
        }

        glBindVertexArray(0);
    }
};
