#include "BeCamera.h"

void BeCamera::updateMatrices() {
    // Update direction vectors
    const float cy = cos(glm::radians(Yaw));
    const float sy = sin(glm::radians(Yaw));
    const float cp = cos(glm::radians(Pitch));
    const float sp = sin(glm::radians(Pitch));

    _front = glm::normalize(glm::vec3(cy * cp, sp, sy * cp));
    _right = glm::normalize(glm::cross(_front, glm::vec3(0, 1, 0)));
    _up = glm::normalize(glm::cross(_right, _front));

    // Update view and projection matrices
    _viewMatrix = glm::lookAtLH(Position, Position + _front, _up);
    _projectionMatrix = glm::perspectiveFovLH(glm::radians(Fov), Width, Height, NearPlane, FarPlane);
}
