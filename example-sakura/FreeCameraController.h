#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

class BeCamera;
class BeInput;

class FreeCameraController {
    expose
    float MoveSpeed = 5.0f;
    float MouseSensitivity = 0.1f;
    //float MouseSensitivity = 0.05f;
    float GamepadCameraSensitivity = 100.0f;
    float MinFov = 20.0f;
    float MaxFov = 90.0f;
    float ZoomSpeed = 30.0f;
    float MinMoveSpeed = 0.5f;
    float MaxMoveSpeed = 200.0f;

    // Exponential smoothing sharpness (higher = snappier, lower = floatier).
    float PositionSmoothing = 12.0f;
    //float RotationSmoothing = 18.0f;
    float RotationSmoothing = 8.0f;

    explicit FreeCameraController(BeCamera* camera);

    auto Update(float deltaTime, BeInput* input) -> void;

    hide
    BeCamera* _camera;

    glm::vec3 _targetPosition{0.0f};
    float _targetYaw = 0.0f;
    float _targetPitch = 0.0f;
    bool _initialised = false;
};
