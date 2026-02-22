#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

class BeCamera;
class BeInput;

class FreeCameraController {
    expose
    float MoveSpeed = 5.0f;
    float MouseSensitivity = 0.1f;
    float GamepadCameraSensitivity = 100.0f;
    float MinFov = 20.0f;
    float MaxFov = 90.0f;

    explicit FreeCameraController(BeCamera* camera);

    auto Update(float deltaTime, BeInput* input) -> void;

    hide
    BeCamera* _camera;
};
