#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/** Camera movement directions used by ProcessKeyboard(). */
enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

/**
 * Camera
 * ------
 * A first-person free-look camera.
 *
 * - WASD / QE to translate
 * - Mouse to look around
 * - Scroll wheel to zoom (change FOV)
 */
class Camera
{
public:
    // Camera state
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Euler angles (degrees)
    float Yaw;
    float Pitch;

    // Control parameters
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;   // field-of-view in degrees

    explicit Camera(glm::vec3 position  = glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3 worldUp   = glm::vec3(0.0f, 1.0f, 0.0f),
                    float     yaw       = -90.0f,
                    float     pitch     = 0.0f);

    glm::mat4 GetViewMatrix() const;

    void ProcessKeyboard    (CameraMovement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset,
                              bool constrainPitch = true);
    void ProcessMouseScroll (float yoffset);

private:
    void updateCameraVectors();
};
