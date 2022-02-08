// Local Headers
#include "skippex.hpp"
#include "shader.hpp"
#include "evao.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include "Object.hpp"
#include "Spline.hpp"

// System Headers
// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// ImGui Widgets (Implot, ...)
#include <implot.h>
#include <implot_internal.h>

// GLAD, GLFW & GLM
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Standard Headers
#include <cstdio>
#include <chrono>  
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <memory>
#include <limits>

// Define Useful Variables and macros
#define VSYNC GL_TRUE
#define FULLSCREEN GL_FALSE
#define RESIZABLE GL_FALSE
#define ACTIVATE_DEBUG false
int width = 1920;
int height = 1080;
float fovDeg = 60.f;
unsigned int samples = 4;
double xpos, ypos;
bool leftMouse = false;

float rectangleVertices[] =
{
        // Coords    // texCoords
        1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,

        1.0f,  1.0f,  1.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
};

// Global GLFW Window
GLFWwindow* window;
GLFWmonitor* monitor;

// Callbacks
static void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

// Function prototypes
void input(Camera& camera);

// States
bool active_mouse = false;
bool showIntersected = false;
bool polling_points = false;
bool useSpheres = false;
int switch_front_back = -1.f;
std::vector<glm::vec3> points_buffer;
std::vector<float> points;
std::vector<float> intersected_points;
std::vector<float> intersectStates;
std::vector<float> intersectSwitches;
std::vector<Sphere> bounding_spheres;
std::vector<glm::mat4> instanceMatrix;
Model spheres;
Curve* curve;
std::vector<sObject> boundingObjects;
bool useInterpolated = true;

void renderLines(bool intersect);
void renderLinesOnSphere(bool intersect, Camera& cam, glm::vec3 hitPos, glm::vec3 hitNormal, glm::mat4 model);
void addSphereInstance(glm::vec3 hitPos, glm::vec3 hitNormal, float size=0.1f, float distance=0.5f);

void updateSphereInstances(glm::vec3 pos, float size=0.1, float height=0.5f)
{
    std::vector<glm::mat4> interpolated_instanceMatrix;
    for (int i = 0; i < instanceMatrix.size(); i++)
    {
        glm::vec3 tempTranslation = bounding_spheres[i].origin * height;
        glm::quat tempRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 tempScale = glm::vec3(size, size, size);

        glm::mat4 trans = glm::mat4(1.0f);
        glm::mat4 rot = glm::mat4(1.0f);
        glm::mat4 sca = glm::mat4(1.0f);

        trans = glm::translate(trans, tempTranslation);
        rot = glm::mat4_cast(tempRotation);
        sca = glm::scale(sca, tempScale);

        instanceMatrix[i] = trans * rot * sca;
    }
    curve = new BSpline();
    curve->set_steps(5);
    for (int i = 0; i < bounding_spheres.size(); i++)
        curve->add_way_point(bounding_spheres[i].origin * height);

    for (int i = 0; i < curve->node_count(); i++)
    {
        glm::vec3 tempTranslation = curve->node(i) ;
        glm::quat tempRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 tempScale = glm::vec3(size, size, size);

        glm::mat4 trans = glm::mat4(1.0f);
        glm::mat4 rot = glm::mat4(1.0f);
        glm::mat4 sca = glm::mat4(1.0f);

        trans = glm::translate(trans, tempTranslation);
        rot = glm::mat4_cast(tempRotation);
        sca = glm::scale(sca, tempScale);

        interpolated_instanceMatrix.push_back(trans * rot * sca);
    }
    if (useInterpolated)
    {
        spheres = Model(pos, glm::vec3(size * 0.1), true, interpolated_instanceMatrix.size(), interpolated_instanceMatrix);
        spheres.loadModel("uvsphere/uvsphere.obj");
    }
    else
    {
        spheres = Model(pos, glm::vec3(size * 0.1), true, instanceMatrix.size(), instanceMatrix);
        spheres.loadModel("uvsphere/uvsphere.obj");
    }

}

void replayCamWithDrawing(Camera& cam)
{
    if (curve->is_empty())
    {
        std::cout << "Empty Curve" << std::endl;
        return;
    }
    cam.positions.clear();
    cam.orientations.clear();

    Curve* detailed_curve = new BSpline();
    detailed_curve->set_steps(20);
    for (int i = 0; i < curve->node_count(); i++)
        detailed_curve->add_way_point(curve->node(i));

    for (int i = 0; i < detailed_curve->node_count() - 1; i++) {
        auto currPos = detailed_curve->node(i);
        auto nextPos = detailed_curve->node(i + 1);
        cam.positions.push_back(currPos);
        cam.orientations.push_back(glm::normalize(nextPos - currPos));
    }
}

int main() {
    
    // Load GLFW and Create a Window
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, RESIZABLE);
    // glfwWindowHint(GLFW_SAMPLES, samples);
    if (ACTIVATE_DEBUG)
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);


    // Setup Monitor to Primary
    monitor = glfwGetPrimaryMonitor();

    // Handle case where we want to render at the maximum monitor resolution aka Fullscreen :-)
    if (FULLSCREEN) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
    }
    std::cout << "Using Window Size : " << width << " x " << height << std::endl;


    // Initialize Window
    window = glfwCreateWindow(width, height, "Skippex | OpenGL", nullptr, nullptr);

    // Check for Valid Context
    if (window == nullptr) {
        fprintf(stderr, "Failed to Create OpenGL Context");
        return EXIT_FAILURE;
    }

    // Create Context and Load OpenGL Functions
    glfwMakeContextCurrent(window);
    gladLoadGL();
    fprintf(stderr, "OpenGL %s\n", glGetString(GL_VERSION));

    // Setting up Callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetErrorCallback(error_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);


    // Enable or Disable VSYNC
    glfwSwapInterval(VSYNC);

    // Enabling DEPTH
    // Enable DEPTH_TEST
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);

    // Enable Multisampling
    // glEnable(GL_MULTISAMPLE);
    // Enables Gamma Correction
    // glEnable(GL_FRAMEBUFFER_SRGB);

    // IMGUI Stuff

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    if (ACTIVATE_DEBUG) {
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }

    // Define Camera
    Camera camera(width, height, glm::vec3(0.0f, 0.5f, 4.5f), 0.25, 65.f);

    // Define Shaders
    LinkedShader nanosuit_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"),
                                                   shader(GL_FRAGMENT_SHADER, "model.frag") }));
    nanosuit_shader.Compile();

    LinkedShader uvsphere_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "light.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "light.frag") }));
    uvsphere_shader.Compile();

    LinkedShader spheres_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "instance.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "instance.frag") }));
    spheres_shader.Compile();

    LinkedShader plane_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"), // model.vert
                                                       shader(GL_FRAGMENT_SHADER, "model.frag") })); //  model.frag
    plane_shader.Compile();

    LinkedShader ballshader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"), // model.vert
                                                    shader(GL_FRAGMENT_SHADER, "model.frag") })); //  model.frag
    ballshader.Compile();

    LinkedShader framebuffershader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "framebuffer.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "framebuffer.frag") }));
    framebuffershader.Compile();
    framebuffershader.Activate();
    framebuffershader.SetInt("screenTexture", 0);

    /*
    LinkedShader shadowShader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "shadow.vert"),
                                                         shader(GL_FRAGMENT_SHADER, "shadow.frag") }));
    shadowShader.Compile();
    */

    // Define Useful variables (time_delta, ImGui elements, etc... )
    auto tchrono_start = std::chrono::high_resolution_clock::now();
    float speed = 1.0f;
    float mscale = 0.5f;
    float lscale = 0.2f;
    float defaultBallScale = 0.05f;
    float plscale = 0.05f;
    float radius = 5.0f;
    float rheight = 0.05f;
    float defaultDrawHeight = 0.5f;
    float ambientStrength = 0.2f;
    float specularStrength = 0.5f;
    float fadeOff = 70.0f;
    bool replay = false;
    float replaySpeed = 1.f;
    bool replayWithDrawing = false;
    int replay_ind = 0;
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.25f, 1.0f);
    ImVec4 mcolor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

    // Plane Variables
    glm::vec4 planeColor = glm::vec4(1.0f);
    glm::vec3 planePos = glm::vec3(0.0f, -1.f, 0.0f);
    glm::mat4 plane_model = glm::mat4(1.0f);
    glm::vec3 rotate_plane(0.f, 90.f, 0.f);
    plane_model = glm::translate(plane_model, planePos);

    // Define Stuff for ImPlot widget
    std::vector<float> x_data(100, 0);
    std::vector<float> y_data(100, 0);
    std::vector<float> z_data(100, 0);
    std::vector<float> t_data;
    std::vector<float> fps_values(100, 0);

    // Lighting Variables
    glm::vec4 lightColor = glm::vec4(1.0f);
    glm::vec3 lightPos = glm::vec3(0.0f, 13.0f, 2.0f);
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);

    glm::vec3 ballPos = glm::vec3 (0, 0, 0);
    glm::quat ballRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    auto size = 1.f;
    glm::vec3 ballScale = glm::vec3(size, size, size);

    glm::mat4 trans = glm::mat4(1.0f);
    glm::mat4 rot = glm::mat4(1.0f);
    glm::mat4 sca = glm::mat4(1.0f);

    trans = glm::translate(trans, ballPos);
    rot = glm::mat4_cast(ballRotation);
    sca = glm::scale(sca, ballScale);

    glm::mat4 bBallModel = trans * rot * sca;
    glm::mat4 bPlaneModel = glm::translate(glm::mat4(1.0f), planePos) * glm::mat4_cast(ballRotation) * glm::mat4(1.0f);
    auto boundingBall = std::make_shared<Sphere>(ballPos, size * 0.50f);
    auto boundingPlane = std::make_shared<Plane>(glm::vec3(0.0f), glm::vec3(0.f, 1.0f, 0.f));

    boundingObjects.push_back(boundingBall);
    //boundingObjects.push_back(boundingPlane);
    /*
    int instances = 2;
    for (int i = -instances; i <= instances; i++) {

        int t = i * 4;
        float r = radius * 0.5;
        // glm::vec3 tempTranslation = glm::vec3 (r * cos(t), 1 + t * rheight * 10, r * sin(t));
        glm::vec3 tempTranslation = glm::vec3 (t, 1, 1);
        glm::quat tempRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        auto size = 1.25f;
        glm::vec3 tempScale = glm::vec3(size, size, size);

        glm::mat4 trans = glm::mat4(1.0f);
        glm::mat4 rot = glm::mat4(1.0f);
        glm::mat4 sca = glm::mat4(1.0f);

        trans = glm::translate(trans, tempTranslation);
        rot = glm::mat4_cast(tempRotation);
        sca = glm::scale(sca, tempScale);

        instanceMatrix.push_back(trans * rot * sca);
        bounding_spheres.emplace_back(tempTranslation, size * 0.595f);
    }*/

    // Define Models get more at https://casual-effects.com/g3d/data10/index.html#mesh4
    Model nanosuit_model(glm::vec3(0.0f, -1, -10), glm::vec3(mscale), false);
    nanosuit_model.loadModel("nanosuit/nanosuit.obj");

    Model plane(planePos, glm::vec3(plscale), false);
    plane.loadModel("Sponza/Sponza.gltf");

    Model uv_sphere(lightPos, glm::vec3(lscale), true);
    uv_sphere.loadModel("uvsphere/uvsphere.obj");

    Model ball(ballPos, glm::vec3(size), true);
    ball.loadModel("uvsphere/uvsphere.obj");

    // Frame Rectangle
    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Create Frame Buffer Object
    unsigned int FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);

    // Create Framebuffer Texture
    unsigned int framebufferTexture;
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferTexture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferTexture, 0);

    // Create Render Buffer Object
    unsigned int RBO;
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    // Error checking framebuffer
    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer error: " << fboStatus << std::endl;

    // Create Non multisamples Frame Buffer Object
    unsigned int postProcessingFBO;
    glGenFramebuffers(1, &postProcessingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, postProcessingFBO);

    // Create Framebuffer Texture
    unsigned int postProcessingTexture;
    glGenTextures(1, &postProcessingTexture);
    glBindTexture(GL_TEXTURE_2D, postProcessingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingTexture, 0);

    // Error checking framebuffer
    fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Post-Processing Framebuffer error: " << fboStatus << std::endl;

    auto t_start = glfwGetTime();

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        // Keep track of elapsed time
        // auto t_now = std::chrono::high_resolution_clock::now();
        // float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

        double t_now = glfwGetTime();
        double time = t_now - t_start;
        t_data.push_back(t_now);

        glClearError();

        // Polling & Updating Elements
        input(camera);
        if (active_mouse) {
            camera.movements(window);
        }
        if (camera.capture)
        {
            replay = false;
            replayWithDrawing = false;
            polling_points = false;
        }
        if (camera.positions.size() > 5) {
            if (replay or replayWithDrawing) {
                replay_ind += int(replaySpeed);
                if (replay_ind >= camera.positions.size())
                    replay_ind = 0;
                camera.P = camera.positions[replay_ind];
                camera.O = camera.orientations[replay_ind];
            }
        }

        camera.update(fovDeg, 0.1f, 500.0f);

        lightPos.x = radius * cos(time * speed);
        lightPos.z = radius * sin(time * speed);
        lightPos.y = rheight * (time * speed);


        // Intersect with ball
        bool intersected = false;
        glm::highp_f32vec3 intersect, normal;
        float t = std::numeric_limits<float>::max();
        Object* intersectedObj;
        Ray ray = camera.getClickDir(int(xpos), int(ypos), width, height);
        for (auto obj : boundingObjects) {
            float curr_t = std::numeric_limits<float>::max();;
            if (obj->get_intersection(ray, intersect, normal, curr_t)) {
                intersected = true;
                if (curr_t <= t) {
                    t = curr_t;
                    intersectedObj = obj.get();
                }
            }
        }
        glm::mat4 intersectedModel;
        if (intersected) {
            if (intersectedObj->type == "Sphere") {
                glm::highp_f32vec4 posIntersect = glm::inverse(bBallModel) * glm::highp_f32vec4(intersect, 1.0f);
                printf("Intersected %s at (%f, %f, %f)\n", intersectedObj->type.c_str(), intersect.x, intersect.y,
                       intersect.z);
                intersectedModel = bBallModel;
            }
            else if (intersectedObj->type == "Plane") {
                glm::highp_f32vec4 posIntersect = glm::inverse(bPlaneModel) * glm::highp_f32vec4(intersect, 1.0f);
                printf("Intersected %s at (%f, %f, %f)\n", intersectedObj->type.c_str(), intersect.x, intersect.y,
                       intersect.z);
                intersectedModel = bPlaneModel;
            }
        }

        /*
        glm::highp_f32vec3 intersect, normal;
        int idSphere = -1;
        bool intersected = false;
        for (int f = 0; f < bounding_spheres.size(); f++) {
            Ray ray = camera.getClickDir(int(xpos), int(ypos), width, height);
            if (bounding_spheres[f].get_intersection(ray, intersect, normal)) {
                intersected = true;
                glm::highp_f32vec4 posIntersect = glm::inverse(instanceMatrix[f]) * glm::highp_f32vec4(intersect, 1.0f);
                printf("Intersected Sphere %d at (%f, %f, %f)\n", f, intersect.x, intersect.y, intersect.z);
                printf("Intersected Sphere %d\n", f);
                idSphere = f;
            }
        }
        */

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);

        glCheckError(); glClearError();

        // Background Fill Color
        if (active_mouse)
            glClearColor(0.15f, 0.15f, 0.15f, 1.f);
        else
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        glCheckError(); glClearError();

        /** Draw Models **/
        // Drawing UV_Sphere as a light
        glm::mat4 plane_model(1.0f);
        plane_model = glm::translate(plane_model, plane.pos);
        plane_model = glm::rotate(plane_model, glm::radians(rotate_plane.x), glm::vec3(1.0f, 0.0f, 0.0f));
        plane_model = glm::rotate(plane_model, glm::radians(rotate_plane.y), glm::vec3(0.0f, 1.0f, 0.0f));
        plane_model = glm::rotate(plane_model, glm::radians(rotate_plane.z), glm::vec3(0.0f, 0.0f, 1.0f));
        plane_model = glm::scale(plane_model, glm::vec3(plscale));
        plane_shader.Activate();

        // Settings Light uniforms
        plane_shader.SetVec3("lightPos", lightPos);
        plane_shader.SetVec4("lightColor", lightColor);
        plane_shader.SetFloat("ambientStrength", ambientStrength);
        plane_shader.SetFloat("specularStrength", specularStrength);
        plane_shader.SetFloat("fadeOff", fadeOff);

        // Settings Model uniforms
        plane_shader.SetMat4("model", plane_model);
        plane_shader.SetMat4("view", camera.view);
        plane_shader.SetMat4("projection", camera.projection);
        plane_shader.SetVec3("cameraPos", camera.P);
        plane_shader.SetFloat("far", camera.far);
        plane_shader.SetFloat("near", camera.near);
        plane_shader.SetVec4("Ucolor", planeColor);
        plane.Draw(plane_shader);

        glCheckError(); glClearError();

        // Drawing UV_Sphere as a light
        glm::mat4 lightModel(1.0f);
        lightModel = glm::translate(lightModel, glm::vec3(lightPos.x, lightPos.y, lightPos.z));
        //lightModel = glm::rotate(lightModel, time * glm::radians(45.f), glm::vec3(0.0f, 1.0f, 0.0f));
        lightModel = glm::scale(lightModel, glm::vec3(lscale));

        uvsphere_shader.Activate();

        // Settings Light uniforms
        uvsphere_shader.SetVec3("lightPos", lightPos);
        uvsphere_shader.SetVec4("lightColor", lightColor);
        uvsphere_shader.SetFloat("ambientStrength", ambientStrength);
        uvsphere_shader.SetFloat("specularStrength", specularStrength);
        uvsphere_shader.SetFloat("fadeOff", fadeOff);

        // Settings Model uniforms
        uvsphere_shader.SetMat4("model", lightModel);
        uvsphere_shader.SetMat4("view", camera.view);
        uvsphere_shader.SetMat4("projection", camera.projection);
        uvsphere_shader.SetVec3("cameraPos", camera.P);
        uvsphere_shader.SetFloat("far", camera.far);
        uvsphere_shader.SetFloat("near", camera.near);
        uv_sphere.Draw(uvsphere_shader);

        glCheckError(); glClearError();

        if (not replayWithDrawing) {
            // Drawing spheres instances
            spheres_shader.Activate();

            // Settings Light uniforms
            spheres_shader.SetVec3("lightPos", lightPos);
            spheres_shader.SetVec4("lightColor", lightColor);
            spheres_shader.SetFloat("ambientStrength", ambientStrength);
            spheres_shader.SetFloat("specularStrength", specularStrength);
            spheres_shader.SetFloat("fadeOff", fadeOff);

            // Settings Model uniforms
            spheres_shader.SetMat4("view", camera.view);
            spheres_shader.SetMat4("projection", camera.projection);
            spheres_shader.SetVec3("cameraPos", camera.P);
            spheres_shader.SetFloat("far", camera.far);
            spheres_shader.SetFloat("near", camera.near);
            spheres_shader.SetVec4("Ucolor", glm::vec4(1.0f));
            spheres.Draw(spheres_shader);
        }
        glCheckError(); glClearError();

        // Drawing Ball
        ballshader.Activate();

        // Settings Light uniforms
        ballshader.SetVec3("lightPos", lightPos);
        ballshader.SetVec4("lightColor", lightColor);
        ballshader.SetFloat("ambientStrength", ambientStrength);
        ballshader.SetFloat("specularStrength", specularStrength);
        ballshader.SetFloat("fadeOff", fadeOff);

        // Settings Model uniforms
        ballshader.SetMat4("model", bBallModel);
        ballshader.SetMat4("view", camera.view);
        ballshader.SetMat4("projection", camera.projection);
        ballshader.SetVec3("cameraPos", camera.P);
        ballshader.SetFloat("far", camera.far);
        ballshader.SetFloat("near", camera.near);
        ball.Draw(ballshader);

        glCheckError(); glClearError();

        // Drawing Nanosuit Model
        glm::mat4 model(1.0f);
        model = glm::translate(model, nanosuit_model.pos);
        // model = glm::rotate(model, time * glm::radians(45.f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(mscale));

        nanosuit_shader.Activate();

        // Settings Light uniforms
        nanosuit_shader.SetVec3("lightPos", lightPos);
        nanosuit_shader.SetVec4("lightColor", lightColor);
        nanosuit_shader.SetFloat("ambientStrength", ambientStrength);
        nanosuit_shader.SetFloat("specularStrength", specularStrength);
        nanosuit_shader.SetFloat("fadeOff", fadeOff);

        // Settings Model uniforms
        nanosuit_shader.SetMat4("model", model);
        nanosuit_shader.SetMat4("view", camera.view);
        nanosuit_shader.SetMat4("projection", camera.projection);
        nanosuit_shader.SetVec3("cameraPos", camera.P);
        nanosuit_shader.SetFloat("far", camera.far);
        nanosuit_shader.SetFloat("near", camera.near);
        nanosuit_shader.SetVec4("Ucolor", glm::vec4(1.0f));
        nanosuit_model.Draw(nanosuit_shader);

        glCheckError(); glClearError();

        if (!active_mouse) {
            std::vector<float> pts;
            if (showIntersected or useSpheres)
                pts = intersected_points;
            else
                pts = points;
            MLine mline(pts);
            mline.setMVP(glm::mat4(1.0f));
            mline.setup();
            mline.draw();
        }

        if (polling_points) {
            if (intersected)
                addSphereInstance(intersect, normal, defaultBallScale, defaultDrawHeight);
            intersectStates.push_back(int(intersected));
            intersectSwitches.push_back(switch_front_back);
            if (useSpheres)
                renderLinesOnSphere(intersected, camera, intersect, normal, intersectedModel);
            else
                renderLines(intersected);
        }

        glCheckError(); glClearError();

        // Bind the default framebuffer

        glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postProcessingFBO);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        // Draw the framebuffer rectangle
        framebuffershader.Activate();
        framebuffershader.SetInt("screenTexture", 0);
        glBindVertexArray(rectVAO);
        glActiveTexture(GL_TEXTURE0 + 0);

        glBindTexture(GL_TEXTURE_2D, postProcessingTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glCheckError(); glClearError();

        // ImGUI Declaration
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        std::reverse(x_data.begin(),x_data.end()); // first becomes last, reverses the vector
        x_data.pop_back();
        std::reverse(x_data.begin(),x_data.end());
        x_data.push_back(lightPos.x);

        std::reverse(y_data.begin(),y_data.end()); // first becomes last, reverses the vector
        y_data.pop_back();
        std::reverse(y_data.begin(),y_data.end());
        y_data.push_back(lightPos.y);

        std::reverse(z_data.begin(),z_data.end()); // first becomes last, reverses the vector
        z_data.pop_back();
        std::reverse(z_data.begin(),z_data.end());
        z_data.push_back(lightPos.z);

        std::reverse(fps_values.begin(),fps_values.end()); // first becomes last, reverses the vector
        fps_values.pop_back();
        std::reverse(fps_values.begin(),fps_values.end());
        auto fps = ImGui::GetIO().Framerate;
        fps_values.push_back(fps);

        glfwSetWindowTitle(window, (std::string("Skippex | OpenG - ") + std::to_string(fps)).c_str());

        // Render Imgui and Implot Widgets
        ImGui::Begin("Change Light and Background");
        ImGui::PlotLines((std::string("FPS : ") + std::to_string(fps_values[fps_values.size() - 1])).c_str(), fps_values.data(), fps_values.size());
        ImGui::Text("Background Settings");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        ImGui::Text("Light Settings");
        ImGui::SliderFloat("scale", &lscale, 0.0f, 1.0f);
        ImGui::SliderFloat("ambientStrength", &ambientStrength, 0.0f, 5.f);
        ImGui::SliderFloat("specularStrength", &specularStrength, 0.0f, 5.f);
        ImGui::SliderFloat("fadeOff", &fadeOff, 0.0f, 1000.0f);
        ImGui::SliderFloat3("position", &lightPos[0], -100, 100);
        ImGui::ColorEdit3("color", (float*)&lightColor);

        ImGui::Text("Light Movement Behaviour");
        ImGui::SliderFloat("Change Radius", &radius, 0.0f, 15.0f);
        ImGui::SliderFloat("Change height", &rheight, 0.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Plane Settings");
        ImGui::Text("Plane Settings");
        ImGui::SliderFloat("scale", &plscale, 0.0f, 1.0f);
        ImGui::SliderFloat3("position", &plane.pos[0], -100, 100);
        ImGui::SliderFloat3("rotate", &rotate_plane[0], 0, 360);
        ImGui::ColorEdit3("color", (float*)&planeColor);
        ImGui::End();

        ImGui::Begin("Selected Model settings");
        ImGui::Text("model settings");
        ImGui::SliderFloat3("position", &nanosuit_model.pos[0], -100, 100);
        ImGui::SliderFloat("scale", &mscale, 0.0f, 5.0f);

        ImGui::Text("Spheres settings");
        auto olddefaultDrawHeight = defaultDrawHeight;
        auto olddefaultBallScale = defaultBallScale;
        ImGui::SliderFloat("default Scale", &defaultBallScale, 0.0f, 1.0f);
        ImGui::SliderFloat("default Height", &defaultDrawHeight, 0.0f, 3.0f);
        if (olddefaultDrawHeight != defaultDrawHeight || olddefaultBallScale != defaultBallScale)
        {
            updateSphereInstances(glm::vec3(0.0f), defaultBallScale, defaultDrawHeight);
        }
        if (ImGui::Button("Update Instances")) {
            updateSphereInstances(glm::vec3(0.0f), defaultBallScale, defaultDrawHeight);
        }

        ImGui::End();

        ImGui::Begin("Camera Settings");
        if (active_mouse)
            ImGui::Text("Camera - Currently Active");
        else
            ImGui::Text("Camera - Currently Passive");
        ImGui::SliderFloat("sensitivity", &camera.sensitivity, 0.0f, 100.0f);
        ImGui::SliderFloat("speed", &camera.speed, 0.0f, 100.0f);
        ImGui::SliderFloat("fovDeg", &fovDeg, 0.0f, 100.0f);
        ImGui::SliderFloat3("position", &camera.P[0], -50.f, 50.f);
        ImGui::Checkbox("Capture", &camera.capture);
        ImGui::Checkbox("Capture Cursor", &leftMouse);
        ImGui::Checkbox("Replay", &replay);
        ImGui::SliderFloat("replaySpeed", &replaySpeed, 1.f, 100.f);
        ImGui::Checkbox("Replay With Drawing", &replayWithDrawing);

        if (replay or replayWithDrawing)
            active_mouse = false;

        if (replayWithDrawing)
            replayCamWithDrawing(camera);

        if (ImGui::Button("reset capture")) {
            camera.positions.clear();
            camera.orientations.clear();
            replay_ind = 0;
            replay = false;
            replayWithDrawing = false;
        }

        if (ImGui::Button("reset position"))
            camera.P = glm::vec3(0.0f, 7.5f, 20.f);
        ImGui::SliderFloat3("orientation", &camera.O[0], -1.f, 1.f);
        if (ImGui::Button("reset orientation"))
            camera.O = glm::vec3(0.0f, 0.0f, -1.0f);
        ImGui::End();

        ImGui::Begin("Plots Window");
        if (ImPlot::BeginPlot("Evolutions of ")) {
            ImPlot::PlotLine("My Line 1", &t_data[0], &x_data[0], x_data.size());
            ImPlot::PlotLine("My Line 2", &t_data[0], &y_data[0], y_data.size());
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("My Plot")) {
            ImPlot::PlotLine("States", &t_data[0], &intersectStates[0], intersectStates.size());
            ImPlot::PlotLine("SwitchesOnOff", &t_data[0], &intersectSwitches[0], intersectSwitches.size());
            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Flip Buffers and Draw
        glfwSwapBuffers(window);

        glfwPollEvents();
    }   
    nanosuit_shader.Delete();
    uvsphere_shader.Delete();
    plane_shader.Delete();
    framebuffershader.Delete();
    ballshader.Delete();
    spheres_shader.Delete();
    // shadowShader.Delete();

    plane.Delete();
    ball.Delete();
    nanosuit_model.Delete();
    uv_sphere.Delete();
    spheres.Delete();

    glDeleteFramebuffers(1, &FBO);
    glDeleteRenderbuffers(1, &RBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

// Callbacks
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void cursor_position_callback(GLFWwindow* window, double x_pos, double y_pos) {
    xpos = x_pos;
    ypos = y_pos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    std::cout << xoffset << std::endl;
}

void input(Camera& camera) {

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS and glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        points_buffer.clear();
        points.clear();
        intersected_points.clear();
        instanceMatrix.clear();
        bounding_spheres.clear();
        intersectStates.clear();
        intersectSwitches.clear();
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            active_mouse = !active_mouse;
            if (active_mouse == true)
            {
                polling_points = false;
                /*
                points_buffer.clear();
                points.clear();
                intersected_points.clear();
                */
            }
        }
    }

    if(!active_mouse && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS and glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        polling_points = true;
    }
    if(!active_mouse && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS and glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        polling_points = false;
    }
    if(!active_mouse && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS and glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        showIntersected = !showIntersected;
    }
    if(!active_mouse && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS and glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        useSpheres = !useSpheres;
        std::cout << "useSpheres : " << useSpheres << std::endl;
    }
    if(!active_mouse && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS and glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
    {
        polling_points = false;
        showIntersected = false;
        useSpheres = false;
        updateSphereInstances(glm::vec3(0.0f), 0.05f);
        std::cout << "useSpheres : " << useSpheres << std::endl;
    }
}

void renderLines(bool intersect)
{
    //Getting cursor position
    glm::vec3 p(0.f,0.f,0.f);
    p.x = xpos;
    p.y = height - ypos;
    p.z = 0;
    // std::cout << "(" << p.x << "," << p.y << ")" << std::endl;

    if (points_buffer.empty()) {
        points_buffer.push_back(p);
    }
    else if (points_buffer.back().x != p.x and points_buffer.back().y != p.y) {
        points_buffer.push_back(p);
        if (points_buffer.size() > 2)
        {
            auto start = points_buffer.at(points_buffer.size() - 2);
            auto end = points_buffer.back();
            float x1 = start.x;
            float y1 = start.y;
            float x2 = end.x;
            float y2 = end.y;
            float w = width;
            float h = height;

            // convert 3d world space position 2d screen space position
            x1 = 2 * x1 / w - 1;
            y1 = 2 * y1 / h - 1;

            x2 = 2 * x2 / w - 1;
            y2 = 2 * y2 / h - 1;

            start.x = x1;
            start.y = y1;
            end.x = x2;
            end.y = y2;

            points.push_back(start.x);
            points.push_back(start.y);
            points.push_back(start.z);
            points.push_back(end.x);
            points.push_back(end.y);
            points.push_back(end.z);

            if (intersect)
            {
                intersected_points.push_back(start.x);
                intersected_points.push_back(start.y);
                intersected_points.push_back(start.z);
                intersected_points.push_back(end.x);
                intersected_points.push_back(end.y);
                intersected_points.push_back(end.z);
            }
        }
    }

    std::cout << "Total Points : " << points_buffer.size() << std::endl;
}

void renderLinesOnSphere(bool intersect, Camera& cam, glm::vec3 hitPos, glm::vec3 hitNormal, glm::mat4 model)
{
    glm::vec4 start = glm::vec4(hitPos, 1.0f);
    glm::vec4 end = glm::vec4(hitPos + hitNormal * 0.5f, 1.0f);
    start = cam.projection * cam.view * model * start;
    end = cam.projection * cam.view * model * end;

    if (intersect)
    {
        points.push_back(start.x);
        points.push_back(start.y);
        points.push_back(start.z);
        points.push_back(end.x);
        points.push_back(end.y);
        points.push_back(end.z);

        intersected_points.push_back(start.x);
        intersected_points.push_back(start.y);
        intersected_points.push_back(start.z);
        intersected_points.push_back(end.x);
        intersected_points.push_back(end.y);
        intersected_points.push_back(end.z);
    }
    std::cout << "Total Points : " << points.size() << std::endl;
}

void addSphereInstance(glm::vec3 hitPos, glm::vec3 hitNormal, float size, float distance)
{
    if (intersected_points.size() < 2)
        return;
    bool OnOff = intersectStates[intersectStates.size() - 1];
    bool prevOnOff = intersectStates[intersectStates.size() - 2];
    if (OnOff != prevOnOff)
        switch_front_back *= -1;
    glm::vec4 pos = glm::vec4(hitPos + hitNormal * distance, 1.0f);
    pos *= switch_front_back;

    glm::vec3 tempTranslation = pos;
    glm::quat tempRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 tempScale = glm::vec3(size, size, size);

    glm::mat4 trans = glm::mat4(1.0f);
    glm::mat4 rot = glm::mat4(1.0f);
    glm::mat4 sca = glm::mat4(1.0f);

    trans = glm::translate(trans, tempTranslation);
    rot = glm::mat4_cast(tempRotation);
    sca = glm::scale(sca, tempScale);

    instanceMatrix.push_back(trans * rot * sca);
    bounding_spheres.emplace_back(tempTranslation, size * 0.595f);
}
