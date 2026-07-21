#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

class BeCamera;
class BeInput;

class ShipCameraController {
    expose
    float PitchRate = 90.0f;
    float YawRate = 70.0f;
    float RollRate = 120.0f;
    float MouseSensitivity = 1.0f;
    float AimRadius = 150.0f;
    float AimDeadZone = 0.06f;
    float MouseReturn = 0.0f;
    float RotationResponse = 6.0f;
    float RollAccel = 2.0f;
    float RollDecel = 4.0f;
    bool InvertPitch = false;

    float ThrustAccel = 40.0f;
    float BoostMultiplier = 3.0f;
    float MaxSpeed = 80.0f;
    float FlightAssistDamping = 2.0f;
    float FullStopDamping = 6.0f;
    bool FlightAssist = true;

    explicit ShipCameraController(BeCamera* camera);

    auto Update(float deltaTime, BeInput* input) -> void;
    auto DrawDebugUI() -> void;
    [[nodiscard]] auto GetAim() const -> glm::vec2 { return _aim; }

    hide
    BeCamera* _camera;
    glm::vec3 _velocity{0.0f};
    glm::vec3 _angularVelocity{0.0f};
    glm::vec2 _aim{0.0f};
};
