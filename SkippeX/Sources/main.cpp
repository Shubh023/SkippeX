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


// Global GLFW Window
GLFWwindow* window;
GLFWmonitor* monitor;


// Callbacks
static void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
    ImGui_ImplOpenGL3_Init("#version 330");

    // Define Camera
    Camera camera(width, height, glm::vec3(0.0f, 7.5f, 20.f), 0.45, 65.f);

    // Define Shaders
    LinkedShader nanosuit_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"),
                                                   shader(GL_FRAGMENT_SHADER, "model.frag") }));
    nanosuit_shader.Compile();

    LinkedShader uvsphere_shader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "light.vert"),
                                                       shader(GL_FRAGMENT_SHADER, "light.frag") }));
    uvsphere_shader.Compile();

    // Define Models
    Model nanosuit_model("nanosuit/nanosuit.obj");
    Model uv_sphere("uvsphere/uvsphere.obj");


    // Define Useful variables (time_delta, ImGui elements, etc... )
    auto t_start = std::chrono::high_resolution_clock::now();
    float speed = 1.0f;
    float mscale = 1.0f;
    float lscale = 0.2f;
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
        camera.update(55.0f, 0.01f, 1000.0f);
        /*
        lightPos = camera.P;
        lightPos.x = radius * cos(time * speed);
        lightPos.z = radius * sin(time * speed);
        lightPos.y = rheight * (time * speed);
        */

        // Background Fill Color
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGUI Declaration
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearError();

        /** Draw Models **/
        // Drawing UV_Sphere as a light
        glm::mat4 sphere_model(1.0f);
        sphere_model = glm::translate(sphere_model, glm::vec3(lightPos.x, lightPos.y, lightPos.z));
        //sphere_model = glm::rotate(sphere_model, time * glm::radians(45.f), glm::vec3(0.0f, 1.0f, 0.0f));
        sphere_model = glm::scale(sphere_model, glm::vec3(lscale));

        uvsphere_shader.Activate();

        glCheckError("uvsphere_shader.Activate();");
        glClearError();

        // Settings Light uniforms
        uvsphere_shader.SetVec3("lightPos", lightPos);
        uvsphere_shader.SetVec4("lightColor", lightColor);
        uvsphere_shader.SetFloat("lightIntensity", lightIntensity);

        // Settings Model uniforms
        uvsphere_shader.SetMat4("model", sphere_model);
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
        ImGui::Begin("Window");
        ImGui::Text("ImGui Window");
        ImGui::SliderFloat("Change speed", &speed, 0.0f, 10.0f);
        ImGui::SliderFloat("Change model scale", &mscale, 0.0f, 1.0f);
        ImGui::SliderFloat("Change light scale", &lscale, 0.0f, 1.0f);
        ImGui::SliderFloat("Change light intensity", &lightIntensity, 0.0f, 100.0f);
        ImGui::SliderFloat("Change Radius", &radius, 0.0f, 15.0f);
        ImGui::SliderFloat("Change height", &rheight, 0.0f, 1.0f);

        // ImGui::Button("Toggle Draw | Explore");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        ImGui::SliderFloat3("translate", &translate[0], -10, 10);
        // ImGui::SliderFloat3("lightPos", &lightPos[0], -20, 20);
        ImGui::ColorEdit3("Light color", (float*)&lightColor);
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


void input(Camera& camera) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            active_mouse = !active_mouse;
            std::cout << active_mouse << std::endl;
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