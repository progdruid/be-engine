#include "OrbitCameraController.h"

#include <umbrellas/include-glfw.h>

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
    , _targetPitch(glm::radians(initialPitch))
    , _orbitRadius(initialRadius)
    , _targetRadius(initialRadius)
    , _targetHeight(lookTarget.y)
{}

auto OrbitCameraController::Update(float deltaTime, BeInput* input) -> void {
    if (input->GetKeyDown(GLFW_KEY_RIGHT)) SpeedMultiplier += SpeedStep;
    if (input->GetKeyDown(GLFW_KEY_LEFT))  SpeedMultiplier -= SpeedStep;
    if (input->GetKeyDown(GLFW_KEY_SPACE)) SpeedMultiplier = 0;
    if (input->GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
        constexpr float pitchSensitivity = 0.005f;
        _targetPitch += input->GetMouseDelta().y * pitchSensitivity;
        _targetPitch = glm::clamp(_targetPitch, glm::radians(-89.0f), glm::radians(89.0f));
    }
    _orbitPitch = glm::mix(_orbitPitch, _targetPitch, 1.0f - glm::exp(-RadiusSmoothSpeed * deltaTime));

    if (input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        constexpr float heightSensitivity = 0.01f;
        _targetHeight -= input->GetMouseDelta().y * heightSensitivity * HeightSpeed;
    }
    _lookTarget.y = glm::mix(_lookTarget.y, _targetHeight, 1.0f - glm::exp(-RadiusSmoothSpeed * deltaTime));

    _orbitAngle += OrbitSpeed * SpeedMultiplier * deltaTime;

    _targetRadius = glm::max(_targetRadius + ScrollSpeed * -input->GetScrollDelta().y, MinRadius);
    _orbitRadius = glm::mix(_orbitRadius, _targetRadius, 1.0f - glm::exp(-RadiusSmoothSpeed * deltaTime));

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
