#pragma once

#include "Mesh.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

using namespace std;



class Model {
public:
    glm::vec3 pos;
    glm::vec3 size;
    bool noTex;

    Model() {};
    Model(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 size = glm::vec3(1.0f), bool noTex = false)
        : pos(pos), size(size), noTex(noTex)
    {};

    void Init() {};
    void loadModel(std::string path) {
        Assimp::Importer import;
        const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "Assimp Error : " << import.GetErrorString() << endl;
            return;
        }

        this->directory = path.substr(0, path.find_last_of('/'));
        this->processNode(scene->mRootNode, scene);
    }

    void Draw(LinkedShader shader) {
        for (Mesh mesh : meshes)
        {
            mesh.Draw(shader);
        }
    }
    void Delete() {
        for (Mesh mesh : meshes) {
            mesh.Delete();
        }
    }

protected:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;

    void processNode(aiNode *node, const aiScene* scene) {
        // Processing all meshes
        for (GLuint i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Processing all child nodes
        for (GLuint i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<GLuint> indices;
        vector<Texture> textures;
        for (GLuint i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

            vertex.Position = glm::vec3(mesh->mVertices[i].x,
                                        mesh->mVertices[i].y,
                                        mesh->mVertices[i].z);

            vertex.Normal = glm::vec3(mesh->mNormals[i].x,
                                        mesh->mNormals[i].y,
                                        mesh->mNormals[i].z);

            if (mesh->mColors[0]) {
                vertex.Color = glm::vec4(mesh->mColors[i]->r,
                                         mesh->mColors[i]->g,
                                         mesh->mColors[i]->b,
                                         mesh->mColors[i]->a);
            }
            else {
                vertex.Color = glm::vec4(1.0f);
            }

            if (mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                             mesh->mTextureCoords[0][i].y);
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f);
            }
            vertices.push_back(vertex);
        }

        // Process indices
        for (GLuint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            if (face.mNumIndices < 3) {
                continue;
            }
            assert(face.mNumIndices == 3);
            for (GLuint j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Process Material
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            if (noTex) {
                // Diffuse Color
                aiColor4D diff(1.0f);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diff);

                // Specular Color
                aiColor4D spec(1.0f);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &spec);

                // Relflective Color
                aiColor4D refl(1.0f);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_REFLECTIVE, &refl);

                return Mesh(vertices, indices, diff, spec, refl);
            }

            vector<Texture> diffuseMaps = loadTextures(material, aiTextureType_DIFFUSE);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            vector<Texture> specularMaps = loadTextures(material, aiTextureType_SPECULAR);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            vector<Texture> reflectionMaps = loadTextures(material, aiTextureType_REFLECTION);
            textures.insert(textures.end(), reflectionMaps.begin(), reflectionMaps.end());
        }
        return Mesh(vertices, indices, textures);

    }


    vector<Texture> loadTextures(aiMaterial* mat, aiTextureType type) {
        vector<Texture> textures;

        for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;

            for (GLuint j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {

                Texture texture(this->directory, str.C_Str(), type);
                texture.load(false);
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

};

GLuint TextureFromFile(const char* path, string directory);


class Modell
{
public:
    glm::vec3 pos;
    glm::vec3 size;

    Modell() {};
    Modell(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 size = glm::vec3(1.0f))
    : pos(pos), size(size)
    {};

    void Draw(LinkedShader shader)
    {
        for (GLuint i = 0; i < this->meshes.size(); i++)
        {
            this->meshes[i].Draw(shader);
        }
    }

    void loadModel(string path)
    {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);

        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "Assimp Error : " << importer.GetErrorString() << endl;
            return;
        }

        this->directory = path.substr(0, path.find_last_of('/'));
        this->processNode(scene->mRootNode, scene);
    }

private:
    string directory;
    vector<Mesh> meshes;
    vector<Texture> textures_loaded;



    void processNode(aiNode *node, const aiScene* scene)
    {
        for ( GLuint i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

            this->meshes.push_back(this->processMesh(mesh, scene));
        }

        for ( GLuint i = 0; i < node->mNumChildren; i++)
        {
            this->processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        vector<Vertex> vertices;
        vector<GLuint> indices;
        vector<Texture> textures;

        for (GLuint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            glm::vec3 vectorN;

            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            if (mesh->mNormals) {
                vectorN.x = mesh->mNormals[i].x;
                vectorN.y = mesh->mNormals[i].y;
                vectorN.z = mesh->mNormals[i].z;
                vertex.Normal = vectorN;
            }

            if (mesh->mColors[0])
            {
                glm::vec4 rgb;

                rgb.x = mesh->mColors[0][i].r;
                rgb.y = mesh->mColors[0][i].g;
                rgb.z = mesh->mColors[0][i].b;
                rgb.w = mesh->mColors[0][i].a;
                vertex.Color = rgb;
            }
            else
                vertex.Color = glm::vec4(1.0f);

            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;

                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }


        for (GLuint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            if (face.mNumIndices < 3) {
                continue;
            }
            assert(face.mNumIndices == 3);
            for (GLuint j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            vector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, aiTextureType_DIFFUSE);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, aiTextureType_SPECULAR);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            vector<Texture> reflectionMaps = this->loadMaterialTextures(material, aiTextureType_REFLECTION, aiTextureType_REFLECTION);
            textures.insert(textures.end(), reflectionMaps.begin(), reflectionMaps.end());
        }
        return Mesh(vertices, indices, textures);
    }

    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, aiTextureType texture_type)
    {
        vector<Texture> textures;

        for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            GLboolean skip = false;

            for (GLuint j = 0; j < textures_loaded.size(); j++)
            {
                if (textures_loaded[j].path == str.C_Str())
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = texture_type;
                texture.path = str.C_Str();
                textures.push_back(texture);

                this->textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};


GLuint TextureFromFile(const char* path, string directory)
{
    string filename = string(path);
    filename = directory + '/' + filename;
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width;
    int height;
    int channels;
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb);

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // std::cout << width << " " <<  height << " " << channels << std::endl;
    stbi_image_free(image);
    return textureID;
}

class Textured {
public:
    Textured(const char* _path, const char* _tex_type, GLenum _tex_id, GLenum _format, GLenum _data_type)
            :   type(_tex_type), tex_id(_tex_id)
    {
        int W, H, C;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* pixels = stbi_load(_path, &W, &H, &C, 0);
        glGenTextures(1, &id);
        glActiveTexture(GL_TEXTURE0 + tex_id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); // GL_REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT_ARB); // GL_REPEAT
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, _format, _data_type, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint id;
    const char*  type;
    GLuint tex_id;

    void assign_unit(Shader& shader, const char* uniform, GLuint tex_id) {
        GLuint texture_uniform = glGetUniformLocation(shader.ID, uniform);
        shader.Activate();
        glUniform1i(texture_uniform, tex_id);
    }
    void bind() {
        glActiveTexture(GL_TEXTURE0 + tex_id);
        glBindTexture(GL_TEXTURE_2D, id);
    }
    void unbind() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    void del() {
        glDeleteTextures(1, &id);
    }
};
