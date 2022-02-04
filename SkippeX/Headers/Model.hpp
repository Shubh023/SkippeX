#pragma once

#include "Mesh.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include <btBulletCollisionCommon.h>
#include "Camera.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

using namespace std;

class Ray {
public:
    Ray(glm::vec3 o = glm::vec3(0.0f), glm::vec3 d = glm::vec3(0.0f))
        : origin(o), direction(d)
    {};
    glm::vec3 origin;
    glm::vec3 direction;
};

class Model;

class World {
public:
    std::vector<btRigidBody*> rigidBodies;
    std::vector<Model*> models;
    btDiscreteDynamicsWorld* dynamicsWorld;
    Camera* camera;

    World(Camera* cam) : camera(cam) {
        btBroadphaseInterface* broadphase = new btDbvtBroadphase();
        btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
        btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
        btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0,-9.81f,0));
    }

    void addRigidBody(Model& model, btRigidBody* rigidBody) {
        models.push_back(&model);
        dynamicsWorld->addRigidBody(rigidBody);
    }

    void updateWorld();

    bool raycast(const glm::vec3& s, glm::vec3& e, glm::vec3& n) {

        btVector3 Start = btVector3(s[0], s[1], s[2]);
        btVector3 End = btVector3(e[0], e[1], e[2]);
        btVector3 Normal = btVector3(n[0], n[1], n[2]);
        btCollisionWorld::ClosestRayResultCallback RayCallback(Start, End);
        RayCallback.m_collisionFilterMask = btBroadphaseProxy::DefaultFilter;

        dynamicsWorld->rayTest(Start, End, RayCallback);
        if(RayCallback.hasHit()) {

            End = RayCallback.m_hitPointWorld;
            Normal = RayCallback.m_hitNormalWorld;
            e = glm::vec3(End[0], End[1], End[2]);
            n = glm::vec3(Normal[0], Normal[1], Normal[2]);
            return true;
        }
        return false;
    }

    glm::mat4 getWorldTransform(glm::mat4 model) {
        return camera->projection * camera->view * model;
    }
};

class Model {
public:
    glm::vec3 position;
    glm::vec3 orientation;
    glm::vec3 scale;
    bool noTex;
    std::vector<Mesh> meshes;
    glm::mat4 model;

    Model() {};
    Model(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f),  bool noTex = false, glm::mat4 model = glm::mat4(1.0f))
        : position(pos), scale(scale), noTex(noTex), model(model)
    {
        orientation = glm::vec3(0.f);
    };

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

    glm::mat4 getModel()
    {
        glm::mat4 nModel(1.0f);
        nModel = glm::translate(nModel, position);
        nModel = glm::rotate(nModel, glm::radians(orientation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        nModel = glm::rotate(nModel, glm::radians(orientation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        nModel = glm::rotate(nModel, glm::radians(orientation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        nModel = glm::scale(nModel, glm::vec3(scale));
        model = nModel;
        return glm::mat4(model);
    }
    glm::mat4 getModelBullet()
    {
        glm::mat4 nModel(1.0f);
        nModel = glm::translate(nModel, glm::vec3(position.x, position.z, -position.y));
        nModel = glm::rotate(nModel, glm::radians(orientation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        nModel = glm::rotate(nModel, glm::radians(orientation.z), glm::vec3(0.0f, 1.0f, 0.0f));
        nModel = glm::rotate(nModel, glm::radians(orientation.y), glm::vec3(0.0f, 0.0f, -1.0f));
        return glm::mat4(nModel);
    }

    void updateModel()
    {
        model = getModel();
    }

    void addRigidBodyToWorld(World& world) {
        auto worldSpaceTransform = world.getWorldTransform(getModel());
        btTriangleMesh* triangleMesh = new btTriangleMesh();
        for (Mesh mesh : meshes) {
            for (int i = 0; i < mesh.indices.size(); i+=3)
            {
                glm::vec3 a = glm::vec3(glm::vec4(mesh.vertices[mesh.indices[i]].Position, 1));
                glm::vec3 b = glm::vec3(glm::vec4(mesh.vertices[mesh.indices[i + 1]].Position, 1));
                glm::vec3 c = glm::vec3(glm::vec4(mesh.vertices[mesh.indices[i + 2]].Position, 1));
                triangleMesh->addTriangle(btVector3(a[0], a[1], a[2]),
                                          btVector3(b[0], b[1], b[2]),
                                          btVector3(c[0], c[1], c[2]));
            }
        }
        btCollisionShape* collisionShape = new btBvhTriangleMeshShape(triangleMesh, false);


        btDefaultMotionState* motionstate = new btDefaultMotionState(btTransform(
                btQuaternion(orientation.x, orientation.z, -orientation.y),
                btVector3(position.x,position.z, -position.y)
        ));

        btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
                0,
                motionstate,
                collisionShape,
                btVector3(0,0,0)
        );

        btRigidBody* rigidBody = new btRigidBody(rigidBodyCI);
        world.rigidBodies.push_back(rigidBody);
        rigidBody->getCollisionShape()->setLocalScaling(btVector3(scale.x, scale.z, scale.y));
        world.addRigidBody(*this, rigidBody);
        rigidBody->setUserPointer((void*)world.rigidBodies.size());
    }
    void Draw(LinkedShader shader) {
        updateModel();
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

void World::updateWorld() {
    for (int i = 0; i < rigidBodies.size(); i++)
    {
        auto m = models[i];
        auto rigidBody = rigidBodies[i];
        auto proj = camera->projection;
        auto view = camera->view;

        glm::mat4 matWithoutScale = proj * view * m->getModel();
        // rigidBody->getCollisionShape()->setLocalScaling(btVector3(m->scale.x, m->scale.y, m->scale.z));
        btTransform transform;
        transform.setFromOpenGLMatrix(glm::value_ptr(m->getModel()));
        rigidBody->setWorldTransform(transform);
    }
}