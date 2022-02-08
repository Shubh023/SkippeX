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
    unsigned int instancing;
    std::vector<glm::mat4> instancesMatrix;

    Model(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 size = glm::vec3(1.0f), bool noTex = false, unsigned int instancing = 1, std::vector<glm::mat4> instancesMatrix = {})
        : pos(pos), size(size), noTex(noTex), instancing(instancing), instancesMatrix(instancesMatrix)
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

                return Mesh(vertices, indices, diff, spec, refl, instancing, instancesMatrix);
            }

            vector<Texture> diffuseMaps = loadTextures(material, aiTextureType_DIFFUSE);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            vector<Texture> specularMaps = loadTextures(material, aiTextureType_SPECULAR);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            vector<Texture> reflectionMaps = loadTextures(material, aiTextureType_REFLECTION);
            textures.insert(textures.end(), reflectionMaps.begin(), reflectionMaps.end());
        }
        return Mesh(vertices, indices, textures, instancing, instancesMatrix);

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
