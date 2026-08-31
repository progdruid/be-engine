#include "ShipCameraController.h"

#include <cmath>

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"
#include "RiftSettings.h"
#include "RiftTerrain.h"
#include "imgui/imgui.h"

ShipCameraController::ShipCameraController(BeCamera* camera, const RiftTerrain* terrain)
    : _camera(camera)
    , _terrain(terrain)
{}

auto ShipCameraController::Update(float deltaTime, BeInput* input) -> void {
    auto& ship = RiftStore::Get().Ship;
    const float dt = deltaTime;

    glm::vec3 targetOmega{0.0f};

    if (_controlsEnabled) {
        const glm::vec2 mouseDelta = input->GetMouseDelta();
        _aim += mouseDelta * (ship.MouseSensitivity / ship.AimRadius);
        const float aimLen = glm::length(_aim);
        if (aimLen > 1.0f) _aim /= aimLen;
        if (ship.MouseReturn > 0.0f) _aim -= _aim * (1.0f - std::exp(-ship.MouseReturn * dt));
        input->SetMouseCapture(true);
    }

    glm::vec2 steer{0.0f};
    const float mag = glm::length(_aim);
    if (mag > ship.AimDeadZone) steer = _aim * ((mag - ship.AimDeadZone) / (1.0f - ship.AimDeadZone) / mag);

    const float pitchSign = ship.InvertPitch ? 1.0f : -1.0f;
    targetOmega.x += pitchSign * steer.y * ship.PitchRate;
    targetOmega.y += steer.x * ship.YawRate;
    if (_controlsEnabled && input->GetKey(GLFW_KEY_A)) targetOmega.z += ship.RollRate;
    if (_controlsEnabled && input->GetKey(GLFW_KEY_D)) targetOmega.z -= ship.RollRate;

    const float rotAlpha = 1.0f - std::exp(-ship.RotationResponse * dt);
    const float rollResponse = std::abs(targetOmega.z) > std::abs(_angularVelocity.z) ? ship.RollAccel : ship.RollDecel;
    const float rollAlpha = 1.0f - std::exp(-rollResponse * dt);
    _angularVelocity.x += (targetOmega.x - _angularVelocity.x) * rotAlpha;
    _angularVelocity.y += (targetOmega.y - _angularVelocity.y) * rotAlpha;
    _angularVelocity.z += (targetOmega.z - _angularVelocity.z) * rollAlpha;
    _camera->RotateLocal(_angularVelocity.x * dt, _angularVelocity.y * dt, _angularVelocity.z * dt);

    if (_isCaptured) {
        const float omega = ship.DockSpringFrequency;
        const float k = omega * omega;
        const float c = 2.0f * ship.DockDampingRatio * omega;
        const glm::vec3 toAnchor = _anchor - _camera->Position;
        _velocity += (k * toAnchor - c * _velocity) * dt;
    } else {
        const glm::quat orientation = _camera->GetOrientation();
        const glm::vec3 front = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

        glm::vec3 thrust{0.0f};
        if (_controlsEnabled) {
            if (input->GetKey(GLFW_KEY_W)) thrust += front;
            if (input->GetKey(GLFW_KEY_S)) thrust -= front;
            if (input->GetKey(GLFW_KEY_Q)) thrust -= up;
            if (input->GetKey(GLFW_KEY_E)) thrust += up;
        }

        float accel = ship.ThrustAccel;
        if (_controlsEnabled && input->GetKey(GLFW_KEY_LEFT_SHIFT)) accel *= ship.BoostMultiplier;

        if (glm::length(thrust) > 0.0001f)
            _velocity += glm::normalize(thrust) * accel * dt;

        if (_controlsEnabled && input->GetKeyDown(GLFW_KEY_SPACE)) ship.FlightAssist = !ship.FlightAssist;

        const float groundHeight = _terrain ? _terrain->GetHeight(_camera->Position.x, _camera->Position.z) : 0.0f;
        const float altitude = _camera->Position.y - groundHeight;
        const float targetProximity = 1.0f - glm::smoothstep(ship.GroundEffectLowAltitude, ship.GroundEffectHighAltitude, altitude);
        _groundEffectProximity += (targetProximity - _groundEffectProximity) * (1.0f - std::exp(-ship.GroundEffectResponse * dt));
        const float proximity = _groundEffectProximity;
        float speedCap = ship.MaxSpeed * glm::mix(ship.GroundEffectSpeedHigh, ship.GroundEffectSpeedLow, proximity);
        
        float damping = 0.0f;
        if (_controlsEnabled && input->GetKey(GLFW_KEY_X)) damping = ship.FullStopDamping;
        else if (ship.FlightAssist) damping = ship.FlightAssistDamping;
        damping *= glm::mix(ship.GroundEffectDragHigh, ship.GroundEffectDragLow, proximity);
        _velocity -= _velocity * (1.0f - std::exp(-damping * dt));

        const float speed = glm::length(_velocity);
        if (speed > speedCap) _velocity *= speedCap / speed;
    }

    _camera->Position += _velocity * dt;

    _lastImpactSpeed = 0.0f;
    if (!_isCaptured && _terrain) {
        const auto collision = _terrain->CollideSphere(_camera->Position, ship.CollisionRadius);
        if (collision.Hit) {
            _camera->Position = collision.Position;
            const float into = glm::dot(_velocity, collision.Normal);
            if (into < 0.0f) {
                _lastImpactSpeed = -into;
                _velocity -= into * collision.Normal;
            }
            _velocity -= _velocity * (1.0f - std::exp(-ship.GroundFriction * dt));
        }
    }

    _camera->Update();
}

auto ShipCameraController::Respawn(glm::vec3 position) -> void {
    _camera->Position = position;
    _velocity = glm::vec3(0.0f);
    _angularVelocity = glm::vec3(0.0f);
    _aim = glm::vec2(0.0f);
    _lastImpactSpeed = 0.0f;
    _camera->Update();
}

auto ShipCameraController::DrawDebugUI() -> void {
    const auto& ship = RiftStore::Get().Ship;
    ImGui::Begin("Ship Camera");
    ImGui::Text("Flight Assist: %s  (Space toggles)", ship.FlightAssist ? "ON" : "OFF");
    const float groundHeight = _terrain ? _terrain->GetHeight(_camera->Position.x, _camera->Position.z) : 0.0f;
    const float altitude = _camera->Position.y - groundHeight;
    const float proximity = _groundEffectProximity;
    ImGui::Text("Altitude: %.0f  Ground Effect: %.0f%%", altitude, proximity * 100.0f);
    ImGui::Text("Speed: %.1f / %.0f", glm::length(_velocity), ship.MaxSpeed * glm::mix(1.0f, ship.GroundEffectSpeedLow, proximity));
    ImGui::Text("Velocity : %+.1f %+.1f %+.1f", _velocity.x, _velocity.y, _velocity.z);
    ImGui::Text("AngVel   : P%+.0f Y%+.0f R%+.0f", _angularVelocity.x, _angularVelocity.y, _angularVelocity.z);
    ImGui::Text("Aim      : %+.2f %+.2f", _aim.x, _aim.y);
    ImGui::TextUnformatted("Mouse = yaw/pitch, A/D = roll.");
    ImGui::TextUnformatted("W/S = thrust, Q/E = up/down.");
    ImGui::TextUnformatted("Shift = boost, Space = toggle flight assist.");
    ImGui::TextUnformatted("C = undock from a station, S = toggle station menu.");
    ImGui::End();
}
