// Local Headers
#include "skippex.hpp"
#include "shader.hpp"
#include "evao.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Camera.hpp"

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
#include <numeric>


// Define Useful Variables and macros
#define VSYNC GL_TRUE
#define FULLSCREEN GL_FALSE
#define RESIZABLE GL_TRUE
int width = 1920;
int height = 1080;
float zoom = 45.f;
unsigned int samples = 8;
double xpos, ypos;

float rectangle_vertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
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
void glClearError();
void glCheckError(const char* s);

// States
bool active_mouse = false;


void ScreenPosToWorldRay(
        int mouseX, int mouseY,             // Mouse position, in pixels, from bottom-left corner of the window
        int screenWidth, int screenHeight,  // Window size, in pixels
        glm::mat4 ViewMatrix,               // Camera position and orientation
        glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
        glm::vec3& out_origin,              // Ouput : Origin of the ray. /!\ Starts at the near plane, so if you want the ray to start at the camera's position instead, ignore this.
        glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
){

    // The ray Start and End positions, in Normalized Device Coordinates (Have you read Tutorial 4 ?)
    glm::vec4 lRayStart_NDC(
            ((float)mouseX/(float)screenWidth  - 0.5f) * 2.0f, // [0,1024] -> [-1,1]
            ((float)mouseY/(float)screenHeight - 0.5f) * 2.0f, // [0, 768] -> [-1,1]
            -1.0, // The near plane maps to Z=-1 in Normalized Device Coordinates
            1.0f
    );
    glm::vec4 lRayEnd_NDC(
            ((float)mouseX/(float)screenWidth  - 0.5f) * 2.0f,
            ((float)mouseY/(float)screenHeight - 0.5f) * 2.0f,
            0.0,
            1.0f
    );

    // The Projection matrix goes from Camera Space to NDC.
    // So inverse(ProjectionMatrix) goes from NDC to Camera Space.
    glm::mat4 InverseProjectionMatrix = glm::inverse(ProjectionMatrix);

    // The View Matrix goes from World Space to Camera Space.
    // So inverse(ViewMatrix) goes from Camera Space to World Space.
    glm::mat4 InverseViewMatrix = glm::inverse(ViewMatrix);

    glm::vec4 lRayStart_camera = InverseProjectionMatrix * lRayStart_NDC;    lRayStart_camera/=lRayStart_camera.w;
    glm::vec4 lRayStart_world  = InverseViewMatrix       * lRayStart_camera; lRayStart_world /=lRayStart_world .w;
    glm::vec4 lRayEnd_camera   = InverseProjectionMatrix * lRayEnd_NDC;      lRayEnd_camera  /=lRayEnd_camera  .w;
    glm::vec4 lRayEnd_world    = InverseViewMatrix       * lRayEnd_camera;   lRayEnd_world   /=lRayEnd_world   .w;


    // Faster way (just one inverse)
    //glm::mat4 M = glm::inverse(ProjectionMatrix * ViewMatrix);
    //glm::vec4 lRayStart_world = M * lRayStart_NDC; lRayStart_world/=lRayStart_world.w;
    //glm::vec4 lRayEnd_world   = M * lRayEnd_NDC  ; lRayEnd_world  /=lRayEnd_world.w;


    glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
    lRayDir_world = glm::normalize(lRayDir_world);


    out_origin = glm::vec3(lRayStart_world);
    out_direction = glm::normalize(lRayDir_world);
}


int main() {

    
    // Load GLFW and Create a Window
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, RESIZABLE);
    glfwWindowHint(GLFW_SAMPLES, samples);

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
    // Enable Multisampling

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

    // Define Camera
    glm::vec3 camPos = glm::vec3(0.0f, 0.f, 5.f);
    Camera camera(width, height, camPos, 0.25, 65.f);

    // Define World
    World world(&camera);

    // Define Shaders
    LinkedShader nanosuit_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"),
                                                   shader(GL_FRAGMENT_SHADER, "model.frag") }));
    nanosuit_shader.Compile();

    LinkedShader plane_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "model.frag") }));
    plane_shader.Compile();

    LinkedShader uvsphere_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "light.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "light.frag") }));
    uvsphere_shader.Compile();

    LinkedShader framebuffershader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "framebuffer.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "framebuffer.frag") }));
    framebuffershader.Compile();


    // Define Useful variables (time_delta, ImGui elements, etc... )
    auto t_start = std::chrono::high_resolution_clock::now();
    float speed = 1.0f;
    float mscale = 1.0f;
    float lscale = 0.2f;
    float plscale = 0.150f;
    float radius = 10.0f;
    float rheight = 0.05f;
    float ambientStrength = 0.2f;
    float specularStrength = 0.5f;
    float fadeOff = 100.0f;
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.25f, 1.0f);
    ImVec4 mcolor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    std::vector<float> fps_values(50, 0);

    // Plane Variables
    glm::vec4 planeColor = glm::vec4(1.0f);
    glm::vec3 planePos = glm::vec3(0.0f, -0.5f, 0.0f);
    glm::mat4 plane_model = glm::mat4(1.0f);
    glm::vec3 rotate_plane(0.f, 0.f, 0.f);
    plane_model = glm::translate(plane_model, planePos);

    // Define Stuff for ImPlot widget
    std::vector<float> x_data(100, 0);
    std::vector<float> y_data(100, 0);
    std::vector<float> z_data(100, 0);
    std::vector<float> t_data(100, 100);
    std::iota(t_data.begin(), t_data.end(), 0);

    // Lighting Variables
    glm::vec4 lightColor = glm::vec4(1.0f);
    glm::vec3 lightPos = glm::vec3(0.0f, 13.0f, 2.0f);

    // Define Models get more at https://casual-effects.com/g3d/data10/index.html#mesh4
    Model nanosuit_model(glm::vec3(0.0f, 0, 0), glm::vec3(mscale), true);
    nanosuit_model.loadModel("sphere/scene.gltf");
    nanosuit_model.addRigidBodyToWorld(world);

    Model plane(planePos, glm::vec3(plscale), false);
    plane.loadModel("Sponza/Sponza.gltf");
    plane.addRigidBodyToWorld(world);

    Model uv_sphere(lightPos, glm::vec3(lscale), true);
    uv_sphere.loadModel("uvsphere/uvsphere.obj");

    std::string message;

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        // Keep track of elapsed time
        auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

        // Polling & Updating Elements
        glfwPollEvents();
        input(camera);
        if (active_mouse != 0)
            camera.movements(window);
        camera.update(55.f, 0.01f, 1000.0f);
        /*
        lightPos = camera.P;
        */
        uv_sphere.position.x = radius * cos(time * speed);
        uv_sphere.position.y = radius * sin(time * speed);
        uv_sphere.position.z = rheight * abs(cos(time * speed)) * time * speed;

        world.updateWorld();

        glm::vec3 out_origin;
        glm::vec3 out_direction;
        ScreenPosToWorldRay(
                xpos, ypos,
                width, height,
                camera.view,
                camera.projection,
                out_origin,
                out_direction
        );

        out_direction = out_direction * 500.0f;

        btCollisionWorld::ClosestRayResultCallback RayCallback(btVector3(out_origin.x, out_origin.y, out_origin.z), btVector3(out_direction.x, out_direction.y, out_direction.z));
        world.dynamicsWorld->rayTest(btVector3(out_origin.x, out_origin.y, out_origin.z), btVector3(out_direction.x, out_direction.y, out_direction.z), RayCallback);
        if(RayCallback.hasHit()) {
            std::ostringstream oss;
            oss << "mesh " << (size_t)RayCallback.m_collisionObject->getUserPointer();
            btVector3 End = RayCallback.m_hitPointWorld;
            btVector3 Normal = RayCallback.m_hitNormalWorld;
            printf("End(%d, %d, %d)\n", End.x(), End.y(), End.z());
            message = oss.str();
        }else{
            message = "background";
        }
        btVector3 p0 = world.rigidBodies[0]->getCenterOfMassPosition();
        std::cout << message  << std::endl;
        //printf("(%d, %d, %d)\n", p0.x(), p0.y(), p0.z());

        // Background Fill Color
        if (active_mouse)
            glClearColor(0.15f, 0.15f, 0.15f, 1.f);
        else
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // ImGUI Declaration
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearError();

        /** Draw Models **/
        // Drawing UV_Sphere as a light
        plane_shader.Activate();

        glCheckError("plane_shader.Activate();");
        glClearError();

        // Settings Light uniforms
        plane_shader.SetVec3("lightPos", uv_sphere.position);
        plane_shader.SetVec4("lightColor", lightColor);
        plane_shader.SetFloat("ambientStrength", ambientStrength);
        plane_shader.SetFloat("specularStrength", specularStrength);
        plane_shader.SetFloat("fadeOff", fadeOff);

        // Settings Model uniforms
        plane_shader.SetMat4("model", plane.getModel());
        plane_shader.SetMat4("view", camera.view);
        plane_shader.SetMat4("projection", camera.projection);
        plane_shader.SetMat4("invProjectionView", glm::inverse(camera.projection * camera.view));
        plane_shader.SetVec3("windowDimensions", glm::vec3(width, height, 1.0f));
        plane_shader.SetVec3("mousePos", glm::vec3(xpos, ypos, 1.0f));
        plane_shader.SetFloat("near", camera.near);
        plane_shader.SetFloat("far", camera.far);
        plane_shader.SetVec3("cameraPos", camera.P);
        plane_shader.SetVec4("Ucolor", planeColor);
        plane.Draw(plane_shader);
        glCheckError("plane_shader.Draw");
        glClearError();

        // Drawing UV_Sphere as a light
        uvsphere_shader.Activate();

        glCheckError("uvsphere_shader.Activate();");
        glClearError();

        // Settings Light uniforms
        uvsphere_shader.SetVec3("lightPos", uv_sphere.position);
        uvsphere_shader.SetVec4("lightColor", lightColor);
        uvsphere_shader.SetFloat("ambientStrength", ambientStrength);
        uvsphere_shader.SetFloat("specularStrength", specularStrength);
        uvsphere_shader.SetFloat("fadeOff", fadeOff);


        // Settings Model uniforms
        uvsphere_shader.SetMat4("model", uv_sphere.getModel());
        uvsphere_shader.SetMat4("view", camera.view);
        uvsphere_shader.SetMat4("projection", camera.projection);
        uvsphere_shader.SetVec3("cameraPos", camera.P);
        uv_sphere.Draw(uvsphere_shader);
        glCheckError("uvsphere_shader.Draw");
        glClearError();

        // Drawing Nanosuit Model
        nanosuit_shader.Activate();

        glCheckError("modelshader.Activate();");
        glClearError();

        // Settings Light uniforms
        nanosuit_shader.SetVec3("lightPos", uv_sphere.position);
        nanosuit_shader.SetVec4("lightColor", lightColor);
        nanosuit_shader.SetFloat("ambientStrength", ambientStrength);
        nanosuit_shader.SetFloat("specularStrength", specularStrength);
        nanosuit_shader.SetFloat("fadeOff", fadeOff);

        // Settings Model uniforms
        nanosuit_shader.SetMat4("model", nanosuit_model.getModel());
        nanosuit_shader.SetMat4("view", camera.view);
        nanosuit_shader.SetMat4("projection", camera.projection);
        nanosuit_shader.SetVec3("cameraPos", camera.P);
        nanosuit_shader.SetVec4("Ucolor", glm::vec4(1.0f));
        nanosuit_shader.SetMat4("invProjectionView", glm::inverse(camera.projection * camera.view));
        nanosuit_shader.SetVec3("windowDimensions", glm::vec3(width, height, 1.0f));
        nanosuit_shader.SetVec3("mousePos", glm::vec3(xpos, ypos, 1.0f));
        nanosuit_shader.SetFloat("near", camera.near);
        nanosuit_shader.SetFloat("far", camera.far);
        nanosuit_model.Draw(nanosuit_shader);
        glCheckError("nanosuit_model.Draw");
        glClearError();

        // std::cout << camera.O.x << " " << camera.O.y << " " << camera.O.z << std::endl;

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
        fps_values.push_back(ImGui::GetIO().Framerate);


        // Render Imgui and Implot Widgets
        ImGui::Begin("Change Light and Background");
        ImGui::PlotLines((std::string("FPS : ") + std::to_string(fps_values[fps_values.size() - 1])).c_str(), fps_values.data(), fps_values.size());
        ImGui::Text("Background Settings");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        ImGui::Text("Light Settings");
        ImGui::SliderFloat("ambientStrength", &ambientStrength, 0.0f, 5.f);
        ImGui::SliderFloat("specularStrength", &specularStrength, 0.0f, 5.f);
        ImGui::SliderFloat("fadeOff", &fadeOff, 0.0f, 1000.0f);
        ImGui::SliderFloat3("position", &uv_sphere.position[0], -10, 10);
        ImGui::SliderFloat3("scale", &uv_sphere.scale[0], 0, 2.0f);
        ImGui::SliderFloat3("orientation", &uv_sphere.orientation[0], 0, 360);
        ImGui::ColorEdit3("color", (float*)&lightColor);

        ImGui::Text("Light Movement Behaviour");
        ImGui::SliderFloat("Change Radius", &radius, 0.0f, 15.0f);
        ImGui::SliderFloat("Change height", &rheight, 0.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Plane Settings");
        ImGui::Text("Plane Settings");
        ImGui::SliderFloat("scale", &plscale, 0.0f, 1.0f);
        ImGui::SliderFloat3("position", &plane.position[0], -10, 10);
        ImGui::SliderFloat3("scale", &plane.scale[0], 0, 2.0f);
        ImGui::SliderFloat3("orientation", &plane.orientation[0], 0, 360);
        ImGui::ColorEdit3("color", (float*)&planeColor);
        ImGui::End();

        plane.scale = glm::vec3(plscale);


        ImGui::Begin("Selected Model settings");
        ImGui::Text("Model");
        ImGui::SliderFloat3("position", &nanosuit_model.position[0], -10, 10);
        ImGui::SliderFloat3("scale", &nanosuit_model.scale[0], 0, 2.0f);
        ImGui::SliderFloat3("orientation", &nanosuit_model.orientation[0], 0, 360);
        ImGui::End();


        ImGui::Begin("Camera Settings");
        ImGui::Text("Camera");
        ImGui::SliderFloat("sensitivity", &camera.sensitivity, 0.0f, 100.0f);
        ImGui::SliderFloat("speed", &camera.speed, 0.0f, 100.0f);
        ImGui::SliderFloat3("position", &camera.P[0], -50.f, 50.f);
        if (ImGui::Button("reset position"))
            camera.P = camPos;
        ImGui::SliderFloat3("orientation", &camera.O[0], -1.f, 1.f);
        if (ImGui::Button("reset orientation"))
            camera.O = glm::vec3(0.0f, 0.0f, -1.0f);
        ImGui::End();


        ImGui::Begin("My Window");
        if (ImPlot::BeginPlot("My Plot")) {
            ImPlot::PlotLine("My Line 1", &t_data[0], &x_data[0], x_data.size());
            ImPlot::PlotLine("My Line 2", &t_data[0], &y_data[0], y_data.size());
            ImPlot::PlotLine("My Line 3", &t_data[0], &z_data[0], z_data.size());
            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glCheckError("Before Swap");
        glClearError();
        // Flip Buffers and Draw
        glfwSwapBuffers(window);
    }   
    nanosuit_shader.Delete();
    uvsphere_shader.Delete();
    plane_shader.Delete();
    framebuffershader.Delete();
    plane.Delete();
    nanosuit_model.Delete();
    uv_sphere.Delete();



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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    std::cout << xoffset << std::endl;
}

void cursor_position_callback(GLFWwindow* window, double x_pos, double y_pos) {
    xpos = x_pos;
    ypos = y_pos;
}


void input(Camera& camera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            active_mouse = !active_mouse;
            // std::cout << active_mouse << std::endl;
        }
    }
}

void glClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

void glCheckError(const char* s)
{
    while (GLenum error = glGetError())
        std::cout << "Opengl error : (" << error << ")" << " at " << s << std::endl;
    glClearError();
}