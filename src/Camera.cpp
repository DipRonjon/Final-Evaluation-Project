#include "Camera.h"

#include <algorithm>

static constexpr float kDefaultSpeed       = 10.0f;
static constexpr float kDefaultSensitivity = 0.1f;
static constexpr float kDefaultZoom        = 45.0f;

Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch)
    : Position(position)
    , Front(glm::vec3(0.0f, 0.0f, -1.0f))
    , WorldUp(worldUp)
    , Yaw(yaw)
    , Pitch(pitch)
    , MovementSpeed(kDefaultSpeed)
    , MouseSensitivity(kDefaultSensitivity)
    , Zoom(kDefaultZoom)
{
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    switch (direction) {
        case CameraMovement::FORWARD:  Position += Front   * velocity; break;
        case CameraMovement::BACKWARD: Position -= Front   * velocity; break;
        case CameraMovement::LEFT:     Position -= Right   * velocity; break;
        case CameraMovement::RIGHT:    Position += Right   * velocity; break;
        case CameraMovement::UP:       Position += WorldUp * velocity; break;
        case CameraMovement::DOWN:     Position -= WorldUp * velocity; break;
    }
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
        Pitch = std::clamp(Pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom = std::clamp(Zoom - yoffset, 1.0f, 90.0f);
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
    front.y = std::sin(glm::radians(Pitch));
    front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));

    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
