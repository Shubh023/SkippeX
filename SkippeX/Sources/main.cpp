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
int width = 800;
int height = 600;


// Global GLFW Window
GLFWwindow* window;
GLFWmonitor* monitor;


// Callbacks
static void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// Function prototypes
void KeyCallback( GLFWwindow *window, int key, int scancode, int action, int mode );
void MouseCallback( GLFWwindow *window, double xPos, double yPos );
void DoMovement( );

// Camera
Camera camera( glm::vec3( 0.0f, 0.0f, 3.0f ) );
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
bool active_mouse = false;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

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
    glfwSetKeyCallback( window, KeyCallback );
    glfwSetCursorPosCallback( window, MouseCallback );

    // Enable or Disable VSYNC
    glfwSwapInterval(VSYNC);

    // IMGUI Stuff
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");


    // Define Shaders
    LinkedShader modelshader(std::vector<shader>({ shader(GL_VERTEX_SHADER, "model.vert"),
                                                   shader(GL_FRAGMENT_SHADER, "model.frag") }));
    modelshader.Compile();


    // Define Models
    Model nanosuit_model("nanosuit/nanosuit.obj");

    // Define Useful variables (time_delta, ImGui elements, etc... )
    auto t_start = std::chrono::high_resolution_clock::now();
    float speed = 1.0;
    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    ImVec4 mcolor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    ImVec4 scale = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    ImVec4 translate = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);


    // Define Stuff for ImPlot widget
    std::vector<float> y_data(255, 0);
    std::vector<float> x_data(100);
    std::iota(x_data.begin(), x_data.end(), 0);

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        // Keep track of elapsed time
        auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

        // Polling & Updating Elements
        glfwPollEvents();
        DoMovement( );
        processInput(window);

        // Background Fill Color
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear( GL_COLOR_BUFFER_BIT);

        // ImGUI Declaration
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        // Draw Models
        glm::mat4 model(1.0f);
        // model = glm::rotate(model, time * glm::radians(45.f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(abs(sin(time * speed))));
        glm::mat4 projection = glm::perspective( camera.GetZoom( ), ( float )width/( float )height, 0.1f, 100.0f );
        glm::mat4 view = camera.GetViewMatrix( );

        modelshader.Activate();

        modelshader.SetMat4("model", model);
        modelshader.SetMat4("view", view);
        modelshader.SetMat4("projection", projection);
        nanosuit_model.Draw(modelshader);


        // Recompute certains components
        for (int i = 0; i < y_data.size(); i++)
        {
            y_data[i] = (sin(clear_color.x * x_data[i]) + cos(clear_color.y * x_data[i])) * cos(time) * speed;
        }


        // Render Imgui and Implot Widgets
        ImGui::Begin("Window");
        ImGui::Text("ImGui Window");
        ImGui::SliderFloat("Change speed", &speed, 0.0f, 10.0f);
        ImGui::Button("Toggle Draw | Explore");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        ImGui::ColorEdit3("scale", (float*)&scale);
        ImGui::ColorEdit3("translate", (float*)&translate);
        ImGui::ColorEdit3("mcolor", (float*)&mcolor);
        ImGui::End();

        ImGui::Begin("My Window");
        if (ImPlot::BeginPlot("My Plot")) {
            ImPlot::PlotLine("My Line Plot", &x_data[0], &y_data[0], x_data.size());
            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Flip Buffers and Draw
        glfwSwapBuffers(window);
    }   
    // shaders.Delete();
    modelshader.Delete();


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

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


// Moves/alters the camera positions based on user input
void DoMovement( )
{
    // Camera controls
    if ( keys[GLFW_KEY_W] || keys[GLFW_KEY_UP] )
    {
        camera.ProcessKeyboard( FORWARD, deltaTime );
    }

    if ( keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN] )
    {
        camera.ProcessKeyboard( BACKWARD, deltaTime );
    }

    if ( keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT] )
    {
        camera.ProcessKeyboard( LEFT, deltaTime );
    }

    if ( keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT] )
    {
        camera.ProcessKeyboard( RIGHT, deltaTime );
    }
}

// Is called whenever a key is pressed/released via GLFW
void KeyCallback( GLFWwindow *window, int key, int scancode, int action, int mode )
{
    if ( GLFW_KEY_ESCAPE == key && GLFW_PRESS == action )
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if ( key >= 0 && key < 1024 )
    {
        if ( action == GLFW_PRESS )
        {
            keys[key] = true;
        }
        else if ( action == GLFW_RELEASE )
        {
            keys[key] = false;
        }
    }

    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
        active_mouse = true;
    else
        active_mouse = false;
}

void MouseCallback( GLFWwindow *window, double xPos, double yPos )
{
    if (active_mouse) {
        if (firstMouse) {
            lastX = xPos;
            lastY = yPos;
            firstMouse = false;
        }

        GLfloat xOffset = xPos - lastX;
        GLfloat yOffset = lastY - yPos;

        lastX = xPos;
        lastY = yPos;


        camera.ProcessMouseMovement(xOffset, yOffset);
    }
}
