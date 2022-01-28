#pragma once

// Std. Includes
#include <vector>

// GL Includes
#define GLEW_STATIC
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const GLfloat YAW        = -90.0f;
const GLfloat PITCH      =  0.0f;
const GLfloat SPEED      =  6.0f;
const GLfloat SENSITIVTY =  0.25f;
const GLfloat ZOOM       =  45.0f;

// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera2
{
public:
    // Constructor with vectors
    Camera2( glm::vec3 position = glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3 up = glm::vec3( 0.0f, 1.0f, 0.0f ), GLfloat yaw = YAW, GLfloat pitch = PITCH ) : front( glm::vec3( 0.0f, 0.0f, -1.0f ) ), movementSpeed( SPEED ), mouseSensitivity( SENSITIVTY ), zoom( ZOOM )
    {
        this->position = position;
        this->worldUp = up;
        this->yaw = yaw;
        this->pitch = pitch;
        this->updateCameraVectors( );
    }

    // Constructor with scalar values
    Camera2( GLfloat posX, GLfloat posY, GLfloat posZ, GLfloat upX, GLfloat upY, GLfloat upZ, GLfloat yaw, GLfloat pitch ) : front( glm::vec3( 0.0f, 0.0f, -1.0f ) ), movementSpeed( SPEED ), mouseSensitivity( SENSITIVTY ), zoom( ZOOM )
    {
        this->position = glm::vec3( posX, posY, posZ );
        this->worldUp = glm::vec3( upX, upY, upZ );
        this->yaw = yaw;
        this->pitch = pitch;
        this->updateCameraVectors( );
    }

    // Returns the view matrix calculated using Eular Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix( )
    {
        return glm::lookAt( this->position, this->position + this->front, this->up );
    }

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard( Camera_Movement direction, GLfloat deltaTime )
    {
        GLfloat velocity = this->movementSpeed * deltaTime;

        if ( direction == FORWARD )
        {
            this->position += this->front * velocity;
        }

        if ( direction == BACKWARD )
        {
            this->position -= this->front * velocity;
        }

        if ( direction == LEFT )
        {
            this->position -= this->right * velocity;
        }

        if ( direction == RIGHT )
        {
            this->position += this->right * velocity;
        }
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement( GLfloat xOffset, GLfloat yOffset, GLboolean constrainPitch = true )
    {
        xOffset *= this->mouseSensitivity;
        yOffset *= this->mouseSensitivity;

        this->yaw   += xOffset;
        this->pitch += yOffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if ( constrainPitch )
        {
            if ( this->pitch > 89.0f )
            {
                this->pitch = 89.0f;
            }

            if ( this->pitch < -89.0f )
            {
                this->pitch = -89.0f;
            }
        }

        // Update Front, Right and Up Vectors using the updated Eular angles
        this->updateCameraVectors( );
    }

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll( GLfloat yOffset )
    {

    }

    GLfloat GetZoom( )
    {
        return this->zoom;
    }

    glm::vec3 GetPosition( )
    {
        return this->position;
    }

    glm::vec3 GetFront( )
    {
        return this->front;
    }

private:
    // Camera Attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Eular Angles
    GLfloat yaw;
    GLfloat pitch;

    // Camera options
    GLfloat movementSpeed;
    GLfloat mouseSensitivity;
    GLfloat zoom;

    // Calculates the front vector from the Camera's (updated) Eular Angles
    void updateCameraVectors( )
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos( glm::radians( this->yaw ) ) * cos( glm::radians( this->pitch ) );
        front.y = sin( glm::radians( this->pitch ) );
        front.z = sin( glm::radians( this->yaw ) ) * cos( glm::radians( this->pitch ) );
        this->front = glm::normalize( front );
        // Also re-calculate the Right and Up vector
        this->right = glm::normalize( glm::cross( this->front, this->worldUp ) );  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        this->up = glm::normalize( glm::cross( this->right, this->front ) );
    }
};


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

    glm::vec3 P;
    glm::vec3 O = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 U = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 CM = glm::mat4(1.0f);
    int width, height;
    bool initial = true;
    float speed = 0.1f, sensitivity = 100.f;
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

void Camera::update(float fov, float near, float far) {
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    view = glm::lookAt(P, P + O, U);
    projection = glm::perspective(glm::radians(fov), (float)width / height, near, far);
    CM = projection * view;
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
        P += speed * -glm::normalize(glm::cross(O , U));
    // MOVE RIGHT
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS or glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        P += speed * glm::normalize(glm::cross(O , U));
    // MOVE UP
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        P += speed * U;
    // MOVE DOWN
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        P += speed * -U;

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