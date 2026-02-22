#include "OrbitCameraController.h"

#include <glfw/glfw3.h>

#include "BeCamera.h"
#include "BeInput.h"

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

auto OrbitCameraController::Update(float deltaTime, BeInput* input) -> void {
    if (input->GetKeyDown(GLFW_KEY_RIGHT)) SpeedMultiplier += SpeedStep;
    if (input->GetKeyDown(GLFW_KEY_LEFT))  SpeedMultiplier -= SpeedStep;
    if (input->GetKeyDown(GLFW_KEY_SPACE)) SpeedMultiplier = 0;
    if (input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        constexpr float pitchSensitivity = 0.005f;
        _orbitPitch += input->GetMouseDelta().y * pitchSensitivity;
        _orbitPitch = glm::clamp(_orbitPitch, glm::radians(-89.0f), glm::radians(89.0f));
    }

    _orbitAngle += OrbitSpeed * SpeedMultiplier * deltaTime;

    _orbitRadius = glm::max(_orbitRadius + ScrollSpeed * -input->GetScrollDelta().y, MinRadius);

    _camera->Position = _lookTarget + glm::vec3(
        glm::cos(_orbitAngle) * glm::cos(_orbitPitch) * _orbitRadius,
        glm::sin(_orbitPitch) * _orbitRadius,
        glm::sin(_orbitAngle) * glm::cos(_orbitPitch) * _orbitRadius
    );

    const glm::vec3 lookDir = glm::normalize(_lookTarget - _camera->Position);
    _camera->Yaw   = glm::degrees(glm::atan(lookDir.z, lookDir.x));
    _camera->Pitch = glm::degrees(glm::asin(lookDir.y));

    _camera->Update();
}
