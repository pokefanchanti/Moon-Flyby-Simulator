#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    // Euler Angles
    float Yaw;
    float Pitch;
    
    // Camera options
    float MovementSpeed;
    float Zoom; //fov

    // Orbital Camera attributes
    bool isOrbitalMode;
    float orbitDistance;
    int targetIndex;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix(glm::vec3 targetPos = glm::vec3(0.0f), float targetRadius = 0.0f);

    // Processes input from the keyboard
    void ProcessKeyboard(Camera_Movement direction, float deltaTime, bool isAccelerating);

    // Processes input from the mouse 
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // Processes input from the mouse scroll wheel
    void ProcessMouseScroll(float yoffset);

    // Toggles for your custom modes
    void ToggleOrbitalMode();
    void CycleTarget(int maxTargets);

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};

#endif