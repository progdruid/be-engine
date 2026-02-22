#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

class BeCamera;

class OrbitCameraController {
    expose
    float OrbitSpeed  = 0.3f;
    float ScrollSpeed = 1.0f;
    float MinRadius   = 0.5f;

    explicit OrbitCameraController(
        BeCamera* camera,
        const glm::vec3& lookTarget = { 0.f, 2.f, 0.f },
        float initialRadius  = 7.0f,
        float initialPitch   = 45.0f
    );

    auto Update(float deltaTime, float scrollDeltaY = 0.f) -> void;

    hide
    BeCamera* _camera;
    glm::vec3 _lookTarget;
    float _orbitAngle  = 0.0f;
    float _orbitPitch;
    float _orbitRadius;
};
