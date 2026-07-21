#include "BeCamera.h"

#include <cmath>

#include "BeRenderer.h"


void BeCamera::SetEuler(float yaw, float pitch, float roll) {
    const glm::quat qYaw   = glm::angleAxis(glm::radians(yaw),    glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat qPitch = glm::angleAxis(glm::radians(-pitch), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat qRoll  = glm::angleAxis(glm::radians(roll),   glm::vec3(0.0f, 0.0f, 1.0f));
    Orientation = glm::normalize(qYaw * qPitch * qRoll);
}

glm::vec3 BeCamera::GetEuler() const {
    const glm::vec3 front = Orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    const float yaw   = glm::degrees(std::atan2(front.x, front.z));
    const float pitch = glm::degrees(std::asin(glm::clamp(front.y, -1.0f, 1.0f)));

    const glm::quat yawPitch = glm::angleAxis(glm::radians(yaw),    glm::vec3(0.0f, 1.0f, 0.0f))
                             * glm::angleAxis(glm::radians(-pitch), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 refUp = yawPitch * glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 up    = Orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    const float roll = glm::degrees(std::atan2(glm::dot(glm::cross(refUp, up), front), glm::dot(refUp, up)));

    return glm::vec3(yaw, pitch, roll);
}

void BeCamera::LookIn(const glm::vec3& front, const glm::vec3& worldUp) {
    const glm::vec3 f = glm::normalize(front);
    const glm::vec3 r = glm::normalize(glm::cross(worldUp, f));
    const glm::vec3 u = glm::cross(f, r);
    Orientation = glm::normalize(glm::quat_cast(glm::mat3(r, u, f)));
}

void BeCamera::RotateLocal(float pitchDegrees, float yawDegrees, float rollDegrees) {
    const glm::quat qYaw   = glm::angleAxis(glm::radians(yawDegrees),    glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat qPitch = glm::angleAxis(glm::radians(-pitchDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat qRoll  = glm::angleAxis(glm::radians(rollDegrees),   glm::vec3(0.0f, 0.0f, 1.0f));
    Orientation = glm::normalize(Orientation * (qYaw * qPitch * qRoll));
}

void BeCamera::Update() {
    assert(Width != 0);
    assert(Height != 0);

    Orientation = glm::normalize(Orientation);
    _front = Orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    _right = Orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    _up    = Orientation * glm::vec3(0.0f, 1.0f, 0.0f);

    _viewMatrix = glm::lookAtLH(Position, Position + _front, _up);
    const float fov    = glm::radians(Fov);
    const float width  = static_cast<float>(Width);
    const float height = static_cast<float>(Height);
    _projectionMatrix = glm::perspectiveFovLH_ZO(fov, width, height, NearPlane, FarPlane);
}
