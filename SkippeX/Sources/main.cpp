// Local Headers
#include "skippex.hpp"
#include "shader.hpp"
#include "evao.hpp"

// System Headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <implot_internal.h>

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


// Define Useful Variables and macros
#define VSYNC GL_TRUE
#define FULLSCREEN GL_FALSE
#define RESIZABLE GL_TRUE
int width = 1280;
int height = 800;


// Global GLFW Window
GLFWwindow* window;
GLFWmonitor* monitor;


// Callbacks
static void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);


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

    // IMGUI Stuff
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


    // Define Shaders
    LinkedShader shaders(std::vector<shader>({ shader(GL_VERTEX_SHADER, "file.vert"),
                                               shader(GL_FRAGMENT_SHADER, "file.frag") }));
    shaders.Compile();

    // Vertex Array
    float vertices[] = {
         0.5f,  0.5f, 0.0f, // Top Right
        -0.5f,  0.5f, 0.0f, // Top Left
        -0.5f, -0.5f, 0.0f, // Bottom Left
         0.5f,  -0.5f, 0.0f // Bottom Right
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    // VAO, VBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);


    // Bind VAO
    glBindVertexArray(VAO);

    // Bind VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Set Attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Set UP EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    /*
    
    Vertex vertices[] =
    {
            Vertex{glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f), glm::vec2(0.0f, 0.0f)},
            Vertex{glm::vec3( 0.0f,  0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f), glm::vec2(0.0f, 10.0f)},
            Vertex{glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f), glm::vec2(10.0f, 10.0f)},
    };

    std::vector<Vertex> verts(vertices, vertices + sizeof(vertices) / sizeof(Vertex));
    VBO vbo(verts);
    VAO vao;
    vao.link(vbo, 0, 3, GL_FLOAT, sizeof(float), (void*)0);
    vbo.unbind();
    */



    auto t_start = std::chrono::high_resolution_clock::now();

    shaders.Activate();

    float speed = 1.0;
    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);


    std::array<float, 10>  x_data = {0, 1, 2 , 3, 4, 5, 6 ,7 , 8, 9};
    std::array<float, 10>  y_data = {0, 1, 4 , 9, 16, 25, 36 , 49, 64, 81};

    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        // Keep track of elapsed time
        auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

        processInput(window);

        // Background Fill Color
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw Shapes
        glm::mat4 transform(1.0f);
        transform = glm::rotate(transform, time * glm::radians(45.f), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, glm::vec3(cos(time * speed)));
        shaders.SetMat4("transform", transform);
        // Issue Draw call on the buffer
        // vao.bind();
        glBindVertexArray(VAO);
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);


        glm::mat4 transform2(1.0f);
        transform2 = glm::rotate(transform2, time * glm::radians(45.f), glm::vec3(0.0f, 0.0f, 1.0f));
        transform2 = glm::scale(transform2, glm::vec3(sin(time * speed)));
        shaders.SetMat4("transform", transform2);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(3 * sizeof(float)));

        ImGui::Begin("Window");
        ImGui::Text("ImGui Window");
        ImGui::SliderFloat("Change speed", &speed, 0.0f, 10.0f);
        ImGui::Button("Toggle Draw | Explore");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        ImGui::End();

        for (int i = 0; i < y_data.size(); i++)
        {
            y_data[i] = x_data[i] * x_data[i] * speed;
        }


        ImGui::Begin("My Window");
        if (ImPlot::BeginPlot("My Plot")) {
            ImPlot::PlotLine("My Line Plot", x_data.begin(), y_data.begin(), 10);
            ImPlot::EndPlot();
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Flip Buffers and Draw
        glfwSwapBuffers(window);
        glfwPollEvents();
    }   
    shaders.Delete();


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
