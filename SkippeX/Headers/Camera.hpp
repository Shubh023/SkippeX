#pragma once

//Local Includes
#include "Object.hpp"

// Std. Includes
#include <vector>

// GL Includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

class Camera {
public:

    Camera(int _width, int _height, glm::vec3 _position);
    Camera(int _width, int _height, glm::vec3 _position, float speed, float sensitivity);

    void matrix(Shader &shader, const char *uniform);
    void movements(GLFWwindow* window);
    void update(float fov, float near, float far);
    Ray getClickDir(int x, int y, int width, int height);
    bool capture = false;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> orientations;

    glm::vec3 P;
    glm::vec3 O = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 U = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 CM = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    int width, height;
    bool initial = true;
    float near = 0.1f, far = 100.f;
    float fov;
    float speed = 0.25f, sensitivity = 65.f;
};

Camera::Camera(int _width, int _height, glm::vec3 _P) {
    width = _width;
    height = _height;
    P = _P;
}

Camera::Camera(int _width, int _height, glm::vec3 _P, float _speed, float _sensitivity)
        : Camera(_width, _height, _P)
{
    speed = _speed;
    sensitivity = _sensitivity;
}

void Camera::update(float nfov, float nNear, float nFar) {
    view = glm::mat4(1.0f);
    projection = glm::mat4(1.0f);
    view = glm::lookAt(P, P + O, U);
    fov = nfov;
    near = nNear;
    far = nFar;
    projection = glm::perspective(glm::radians(fov), (float)width / height, near, far);
    CM = projection * view;
    if (capture == true)
    {
        positions.push_back(P);
        orientations.push_back(O);
    }
}

void Camera::matrix(Shader &shader, const char *uniform) {
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform), 1, GL_FALSE, glm::value_ptr(CM));
}

void Camera::movements(GLFWwindow *window) {
    // MOVE FORWARD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS or glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        P += speed * O ;
    // MOVE BACKWARDS
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS or glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        P += speed * -O ;
    // MOVE LEFT
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS or glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        P += speed * -glm::normalize(glm::cross(O , U)) * 0.40f;
    // MOVE RIGHT
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS or glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        P += speed * glm::normalize(glm::cross(O , U)) * 0.40f;
    // MOVE UP
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        P += speed * U * 0.35f;
    // MOVE DOWN
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        P += speed * -U * 0.35f;

    // ROTATE CAMERA with mouse
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        // If we interact with the window we don't want to see out cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        if (initial)
        {
            // When we interact lets make sur the cursor is placed in the center of the window
            // Like a first person shooter kind of centered cursor
            glfwSetCursorPos(window, (width / 2), (height / 2));
            initial = false;
        }
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        // Calculate the amount of rotation per X & Y axis
        float horizontal = sensitivity * (float)(mouseY - (height / 2)) / height;
        float vertical = sensitivity * (float)(mouseX - (width / 2)) / width;
        auto norm = glm::normalize(glm::cross(O , U));
        glm::vec3 tmpO  = glm::rotate(O , glm::radians(-horizontal), norm);
        auto displacement = abs(glm::angle(tmpO , U) - glm::radians(90.0f));
        if (displacement <= glm::radians(80.0f))
            O  = tmpO;
        O  = glm::rotate(O , glm::radians(-vertical), U);
        glfwSetCursorPos(window, (width / 2), (height / 2));
    }
    // Reset Cursor to normal mode if we don't interact with the screen
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        initial = true;
    }
}

Ray Camera::getClickDir(int x, int y, int width, int height) {

    const glm::highp_f32vec2 pos(x,y);
    glm::highp_f32mat4 invMat = glm::inverse(CM);
    float halfScreenWidth = float(width) / 2.f;
    float halfScreenHeight = float(height) / 2.f;
    glm::highp_f32vec4 near = glm::vec4((pos.x - halfScreenWidth) / halfScreenWidth, -1 * (pos.y - halfScreenHeight) / halfScreenHeight, -1, 1.0);
    glm::highp_f32vec4 far = glm::vec4((pos.x - halfScreenWidth) / halfScreenWidth, -1 * (pos.y - halfScreenHeight) / halfScreenHeight, 1, 1.0);
    glm::highp_f32vec4 nearResult = invMat * near;
    glm::highp_f32vec4 farResult = invMat * far;
    nearResult /= nearResult.w;
    farResult /= farResult.w;
    auto res = farResult - nearResult;
    // auto res = glm::vec3(nearResult.x, nearResult.y, nearResult.z) - P;
    glm::highp_f32vec3 dir = glm::vec3(res.x, res.y, res.z);
    dir = normalize(dir);
    return Ray(nearResult, dir);
}