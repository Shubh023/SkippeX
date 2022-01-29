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

// Global GLFW Window
GLFWwindow* window;
GLFWmonitor* monitor;


// Callbacks
static void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Function prototypes
void input(Camera& camera);
void glClearError();
void glCheckError(const char* s);

// States
bool active_mouse = false;

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

    // Enable or Disable VSYNC
    glfwSwapInterval(VSYNC);

    // Enable DEPTH
    glEnable( GL_DEPTH_TEST );
    glDepthMask(GL_TRUE);

    // IMGUI Stuff
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Define Camera
    Camera camera(width, height, glm::vec3(0.0f, 7.5f, 20.f), 0.25, 65.f);

    // Define Shaders
    LinkedShader nanosuit_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"),
                                                   shader(GL_FRAGMENT_SHADER, "model.frag") }));
    nanosuit_shader.Compile();

    LinkedShader plane_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "light.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "light.frag") }));
    plane_shader.Compile();

    LinkedShader uvsphere_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "light.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "light.frag") }));
    uvsphere_shader.Compile();

    // Define Models get more at https://casual-effects.com/g3d/data10/index.html#mesh4
    Model nanosuit_model("nanosuit/nanosuit.obj");
    Model plane("CuriosityQR/CuriosityQR.stl");
    Model uv_sphere("uvsphere/uvsphere.obj");

    // Define Useful variables (time_delta, ImGui elements, etc... )
    auto t_start = std::chrono::high_resolution_clock::now();
    float speed = 1.0f;
    float mscale = 1.0f;
    float lscale = 0.2f;
    float plscale = 0.25f;
    float radius = 10.0f;
    float rheight = 0.05f;
    float lightIntensity = 1.0f;
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.25f, 1.0f);
    ImVec4 mcolor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    glm::vec3 translate(0.0f, -0.5, 0);

    // Define Stuff for ImPlot widget
    std::vector<float> x_data(100, 0);
    std::vector<float> y_data(100, 0);
    std::vector<float> z_data(100, 0);
    std::vector<float> t_data(100, 100);
    std::iota(t_data.begin(), t_data.end(), 0);


    // Lighting Variables
    glm::vec4 lightColor = glm::vec4(1.0f);
    glm::vec3 lightPos = glm::vec3(0.0f, 5.0f, 2.0f);
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);

    // Plane Variables
    glm::vec4 planeColor = glm::vec4(1.0f);
    glm::vec3 planePos = glm::vec3(20.0f, -25.0f, -15.0f);
    glm::mat4 plane_model = glm::mat4(1.0f);
    glm::vec3 rotate_plane(270.f, 0.f, 0.f);
    plane_model = glm::translate(plane_model, planePos);


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
        camera.update(zoom, 0.01f, 1000.0f);
        /*
        lightPos = camera.P;
        lightPos.x = radius * cos(time * speed);
        lightPos.z = radius * sin(time * speed);
        lightPos.y = rheight * (time * speed);
        */

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
        glm::mat4 plane_model(1.0f);
        plane_model = glm::translate(plane_model, glm::vec3(planePos.x, planePos.y, planePos.z));
        plane_model = glm::rotate(plane_model, glm::radians(rotate_plane.x), glm::vec3(1.0f, 0.0f, 0.0f));
        plane_model = glm::rotate(plane_model, glm::radians(rotate_plane.y), glm::vec3(0.0f, 1.0f, 0.0f));
        plane_model = glm::rotate(plane_model, glm::radians(rotate_plane.z), glm::vec3(0.0f, 0.0f, 1.0f));
        plane_model = glm::scale(plane_model, glm::vec3(plscale));

        plane_shader.Activate();

        glCheckError("plane_shader.Activate();");
        glClearError();

        // Settings Light uniforms
        plane_shader.SetVec3("lightPos", lightPos);
        plane_shader.SetVec4("lightColor", planeColor);
        plane_shader.SetFloat("lightIntensity", lightIntensity);

        // Settings Model uniforms
        plane_shader.SetMat4("model", plane_model);
        plane_shader.SetMat4("projection_view", camera.CM);
        plane.Draw(plane_shader);
        glCheckError("plane_shader.Draw");
        glClearError();

        // Drawing UV_Sphere as a light
        glm::mat4 lightModel(1.0f);
        lightModel = glm::translate(lightModel, glm::vec3(lightPos.x, lightPos.y, lightPos.z));
        //lightModel = glm::rotate(lightModel, time * glm::radians(45.f), glm::vec3(0.0f, 1.0f, 0.0f));
        lightModel = glm::scale(lightModel, glm::vec3(lscale));

        uvsphere_shader.Activate();

        glCheckError("uvsphere_shader.Activate();");
        glClearError();

        // Settings Light uniforms
        uvsphere_shader.SetVec3("lightPos", lightPos);
        uvsphere_shader.SetVec4("lightColor", lightColor);
        uvsphere_shader.SetFloat("lightIntensity", lightIntensity);

        // Settings Model uniforms
        uvsphere_shader.SetMat4("model", lightModel);
        uvsphere_shader.SetMat4("projection_view", camera.CM);
        uv_sphere.Draw(uvsphere_shader);
        glCheckError("uvsphere_shader.Draw");
        glClearError();

        // Drawing Nanosuit Model
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(translate.x, translate.y, translate.z));
        // model = glm::rotate(model, time * glm::radians(45.f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(mscale));

        nanosuit_shader.Activate();

        glCheckError("modelshader.Activate();");
        glClearError();

        // Settings Light uniforms
        nanosuit_shader.SetVec3("lightPos", lightPos);
        nanosuit_shader.SetVec4("lightColor", lightColor);
        nanosuit_shader.SetFloat("lightIntensity", lightIntensity);


        // Settings Model uniforms
        nanosuit_shader.SetMat4("model", model);
        nanosuit_shader.SetMat4("projection_view", camera.CM);
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


        // Render Imgui and Implot Widgets
        ImGui::Begin("Change Light and Background");
        ImGui::Text("Background Settings");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        ImGui::Text("Light Settings");
        ImGui::SliderFloat("scale", &lscale, 0.0f, 1.0f);
        ImGui::SliderFloat("intensity", &lightIntensity, 0.0f, 100.0f);
        ImGui::SliderFloat3("position", &lightPos[0], -100, 100);
        ImGui::ColorEdit3("color", (float*)&lightColor);

        ImGui::Text("Light Movement Behaviour");
        ImGui::SliderFloat("Change Radius", &radius, 0.0f, 15.0f);
        ImGui::SliderFloat("Change height", &rheight, 0.0f, 1.0f);
        ImGui::End();

        ImGui::Begin("Plane Settings");
        ImGui::Text("Plane Settings");
        ImGui::SliderFloat("scale", &plscale, 0.0f, 1.0f);
        ImGui::SliderFloat3("position", &planePos[0], -100, 100);
        ImGui::SliderFloat3("rotate", &rotate_plane[0], 0, 360);
        ImGui::ColorEdit3("color", (float*)&planeColor);
        ImGui::End();


        ImGui::Begin("Selected Model settings");
        ImGui::Text("model settings");
        ImGui::SliderFloat3("position", &translate[0], -100, 100);
        ImGui::SliderFloat("scale", &mscale, 0.0f, 5.0f);
        ImGui::End();


        ImGui::Begin("Camera Settings");
        ImGui::Text("Light Movement Behaviour");
        ImGui::SliderFloat("sensitivity", &camera.sensitivity, 0.0f, 100.0f);
        ImGui::SliderFloat("speed", &camera.speed, 0.0f, 100.0f);
        ImGui::SliderFloat3("position", &camera.P[0], -50.f, 50.f);
        if (ImGui::Button("reset position"))
            camera.P = glm::vec3(0.0f, 7.5f, 20.f);
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
    // shaders.Delete();
    nanosuit_shader.Delete();
    uvsphere_shader.Delete();
    plane_shader.Delete();


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