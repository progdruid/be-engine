#include "FreeCameraController.h"

#include <cmath>

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"

FreeCameraController::FreeCameraController(BeCamera* camera)
    : _camera(camera)
{}

auto FreeCameraController::Update(float deltaTime, BeInput* input) -> void {
    // Seed the smoothing targets from the camera's current state on first use,
    // so we don't lerp from an uninitialised origin.
    if (!_initialised) {
        _targetPosition = _camera->Position;
        _targetYaw      = _camera->Yaw;
        _targetPitch    = _camera->Pitch;
        _initialised    = true;
    }

    float speed = MoveSpeed * deltaTime;
    if (input->GetKey(GLFW_KEY_LEFT_SHIFT) || (input->IsGamepadConnected() && input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) speed *= 2.0f;
    if (input->GetKey(GLFW_KEY_W)) _targetPosition += _camera->GetFront() * speed;
    if (input->GetKey(GLFW_KEY_S)) _targetPosition -= _camera->GetFront() * speed;
    if (input->GetKey(GLFW_KEY_D)) _targetPosition -= _camera->GetRight() * speed;
    if (input->GetKey(GLFW_KEY_A)) _targetPosition += _camera->GetRight() * speed;
    if (input->GetKey(GLFW_KEY_E)) _targetPosition += glm::vec3(0, 1, 0) * speed;
    if (input->GetKey(GLFW_KEY_Q)) _targetPosition -= glm::vec3(0, 1, 0) * speed;

    // Gamepad movement
    if (input->IsGamepadConnected()) {
        const glm::vec2 leftStick = input->GetGamepadLeftStick();
        _targetPosition += _camera->GetFront() * (leftStick.y * speed);
        _targetPosition -= _camera->GetRight() * (leftStick.x * speed);

        const float verticalInput = input->GetGamepadRightTrigger() - input->GetGamepadLeftTrigger();
        _targetPosition += glm::vec3(0, 1, 0) * (verticalInput * speed);
    }

    bool captureMouse = false;
    if (input->GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
        captureMouse = true;
        const glm::vec2 mouseDelta = input->GetMouseDelta();
        _targetYaw   -= mouseDelta.x * MouseSensitivity;
        _targetPitch -= mouseDelta.y * MouseSensitivity;
        _targetPitch = glm::clamp(_targetPitch, -89.0f, 89.0f);
    }
    input->SetMouseCapture(captureMouse);

    // Gamepad camera look
    if (input->IsGamepadConnected()) {
        const glm::vec2 rightStick = input->GetGamepadRightStick();
        _targetYaw   -= rightStick.x * GamepadCameraSensitivity * deltaTime;
        _targetPitch += rightStick.y * GamepadCameraSensitivity * deltaTime;
        _targetPitch = glm::clamp(_targetPitch, -89.0f, 89.0f);
    }

    if (input->GetKey(GLFW_KEY_EQUAL))  { _camera->Fov -= ZoomSpeed * deltaTime; _camera->Fov = glm::clamp(_camera->Fov, MinFov, MaxFov); }
    if (input->GetKey(GLFW_KEY_MINUS))  { _camera->Fov += ZoomSpeed * deltaTime; _camera->Fov = glm::clamp(_camera->Fov, MinFov, MaxFov); }

    const glm::vec2 scrollDelta = input->GetScrollDelta();
    if (scrollDelta.y != 0.0f) {
        MoveSpeed *= std::pow(1.2f, scrollDelta.y);
        MoveSpeed = glm::clamp(MoveSpeed, MinMoveSpeed, MaxMoveSpeed);
    }

    // Framerate-independent exponential smoothing toward the targets.
    const float posAlpha = 1.0f - std::exp(-PositionSmoothing * deltaTime);
    const float rotAlpha = 1.0f - std::exp(-RotationSmoothing * deltaTime);
    _camera->Position += (_targetPosition - _camera->Position) * posAlpha;
    _camera->Yaw      += (_targetYaw - _camera->Yaw) * rotAlpha;
    _camera->Pitch    += (_targetPitch - _camera->Pitch) * rotAlpha;

    _camera->Update();
}
