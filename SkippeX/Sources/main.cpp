// Local Headers
#include "skippex.hpp"
#include "shader.hpp"
#include "evao.hpp"

// System Headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Standard Headers
#include <cstdio>
#include <chrono>  
#include <cstdlib>
#include <iostream>


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


    // Enable or Disable VSYNC
    glfwSwapInterval(VSYNC);

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
    // Rendering Loop
    while (!glfwWindowShouldClose(window)) {
        // Keep track of elapsed time
        auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

        processInput(window);

        // Background Fill Color
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        

        // Draw Shapes
        glm::mat4 transform(1.0f);
        transform = glm::rotate(transform, time * glm::radians(45.f), glm::vec3(0.0f, 0.0f, 1.0f));
        shaders.SetMat4("transform", transform);
        // Issue Draw call on the buffer
        // vao.bind();
        glBindVertexArray(VAO);
        // glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Flip Buffers and Draw
        glfwSwapBuffers(window);
        glfwPollEvents();
    }   
    shaders.Delete();

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