#include "FreeCameraController.h"

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"

FreeCameraController::FreeCameraController(BeCamera* camera)
    : _camera(camera)
{}

auto FreeCameraController::Update(float deltaTime, BeInput* input) -> void {
    float speed = MoveSpeed * deltaTime;
    if (input->GetKey(GLFW_KEY_LEFT_SHIFT) || (input->IsGamepadConnected() && input->GetGamepadButton(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER))) speed *= 2.0f;
    if (input->GetKey(GLFW_KEY_W)) _camera->Position += _camera->GetFront() * speed;
    if (input->GetKey(GLFW_KEY_S)) _camera->Position -= _camera->GetFront() * speed;
    if (input->GetKey(GLFW_KEY_D)) _camera->Position -= _camera->GetRight() * speed;
    if (input->GetKey(GLFW_KEY_A)) _camera->Position += _camera->GetRight() * speed;
    if (input->GetKey(GLFW_KEY_E)) _camera->Position += glm::vec3(0, 1, 0) * speed;
    if (input->GetKey(GLFW_KEY_Q)) _camera->Position -= glm::vec3(0, 1, 0) * speed;

    // Gamepad movement
    if (input->IsGamepadConnected()) {
        const glm::vec2 leftStick = input->GetGamepadLeftStick();
        _camera->Position += _camera->GetFront() * (leftStick.y * speed);
        _camera->Position -= _camera->GetRight() * (leftStick.x * speed);

        const float verticalInput = input->GetGamepadRightTrigger() - input->GetGamepadLeftTrigger();
        _camera->Position += glm::vec3(0, 1, 0) * (verticalInput * speed);
    }

    bool captureMouse = false;
    if (input->GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        captureMouse = true;
        const glm::vec2 mouseDelta = input->GetMouseDelta();
        _camera->Yaw   -= mouseDelta.x * MouseSensitivity;
        _camera->Pitch -= mouseDelta.y * MouseSensitivity;
        _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
    }
    input->SetMouseCapture(captureMouse);

    // Gamepad camera look
    if (input->IsGamepadConnected()) {
        const glm::vec2 rightStick = input->GetGamepadRightStick();
        _camera->Yaw   -= rightStick.x * GamepadCameraSensitivity * deltaTime;
        _camera->Pitch += rightStick.y * GamepadCameraSensitivity * deltaTime;
        _camera->Pitch = glm::clamp(_camera->Pitch, -89.0f, 89.0f);
    }

    const glm::vec2 scrollDelta = input->GetScrollDelta();
    if (scrollDelta.y != 0.0f) {
        _camera->Fov -= scrollDelta.y;
        _camera->Fov = glm::clamp(_camera->Fov, MinFov, MaxFov);
    }

    _camera->Update();
}
