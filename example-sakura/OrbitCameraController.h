#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

class BeCamera;
class BeInput;

class OrbitCameraController {
    expose
    float OrbitSpeed  = 0.3f;
    float SpeedMultiplier = 1.0f;
    float SpeedStep = 0.1f;
    float ScrollSpeed = 1.0f;
    float MinRadius   = 0.5f;
    float RadiusSmoothSpeed = 10.0f;
    float HeightSpeed = 5.0f;

    explicit OrbitCameraController(
        BeCamera* camera,
        const glm::vec3& lookTarget = { 0.f, 0.f, 0.f },
        float initialRadius  = 7.0f,
        float initialPitch   = 45.0f
    );

    auto Update(float deltaTime, BeInput* input) -> void;

    hide
    BeCamera* _camera;
    glm::vec3 _lookTarget;
    float _orbitAngle  = 0.0f;
    float _orbitPitch;
    float _targetPitch;
    float _orbitRadius;
    float _targetRadius;
    float _targetHeight;
};
