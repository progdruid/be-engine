#include "OrbitCameraController.h"

#include "BeCamera.h"

OrbitCameraController::OrbitCameraController(
    BeCamera* camera,
    const glm::vec3& lookTarget,
    float initialRadius,
    float initialPitch
)
    : _camera(camera)
    , _lookTarget(lookTarget)
    , _orbitPitch(glm::radians(initialPitch))
    , _orbitRadius(initialRadius)
{}

auto OrbitCameraController::Update(float deltaTime, float scrollDeltaY) -> void {
    _orbitAngle += OrbitSpeed * deltaTime;

    _orbitRadius = glm::max(_orbitRadius + ScrollSpeed * -scrollDeltaY, MinRadius);

    _camera->Position = _lookTarget + glm::vec3(
        glm::cos(_orbitAngle) * _orbitRadius,
        glm::sin(_orbitPitch) * _orbitRadius,
        glm::sin(_orbitAngle) * _orbitRadius
    );

    const glm::vec3 lookDir = glm::normalize(_lookTarget - _camera->Position);
    _camera->Yaw   = glm::degrees(glm::atan(lookDir.z, lookDir.x));
    _camera->Pitch = glm::degrees(glm::asin(lookDir.y));

    _camera->Update();
}
