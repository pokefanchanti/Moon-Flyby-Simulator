#include "../include/Camera.h"
#include <cmath>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(5.0f), Zoom(45.0f), 
      isOrbitalMode(true), orbitDistance(4.0f), targetIndex(0)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix(glm::vec3 targetPos, float targetRadius) {
    if (isOrbitalMode) {
        float minSafeDistance = targetRadius * 1.2f;
        if (orbitDistance < minSafeDistance) {
            orbitDistance = minSafeDistance;
        }
        // Lock camera position relative to the target and front vector
        Position = targetPos - (Front * orbitDistance);
        return glm::lookAt(Position, targetPos, WorldUp);
    } else {
        // Standard Free-Fly
        return glm::lookAt(Position, Position + Front, Up);
    }
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime, bool isAccelerating) {
    if (isOrbitalMode) return; // Disable WASD in orbital mode

    float velocity = MovementSpeed * deltaTime;
    if (isAccelerating) velocity *= 10.0f; // 10x speed boost with shift

    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += WorldUp * velocity;
    if (direction == DOWN)
        Position -= WorldUp * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    // Dynamic sensitivity based on FOV (just like you had it)
    float sensitivity = (0.1f / 45.0f) * Zoom;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    if (isOrbitalMode) {
        float zoomSpeed = orbitDistance * 0.1f;
        orbitDistance -= yoffset * zoomSpeed;
        if (orbitDistance > 500.0f) orbitDistance = 500.0f;
    } else {
        Zoom -= yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }
}

void Camera::ToggleOrbitalMode() {
    isOrbitalMode = !isOrbitalMode;
}

void Camera::CycleTarget(int maxTargets) {
    targetIndex++;
    if (targetIndex >= maxTargets) targetIndex = 0;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    
    // Re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  
    Up    = glm::normalize(glm::cross(Right, Front));
}