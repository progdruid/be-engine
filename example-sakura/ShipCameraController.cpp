#include "ShipCameraController.h"

#include <cmath>

#include <umbrellas/include-glfw.h>

#include "BeCamera.h"
#include "BeInput.h"
#include "imgui/imgui.h"

ShipCameraController::ShipCameraController(BeCamera* camera)
    : _camera(camera)
{}

auto ShipCameraController::Update(float deltaTime, BeInput* input) -> void {
    const float dt = deltaTime;

    glm::vec3 targetOmega{0.0f};

    const glm::vec2 mouseDelta = input->GetMouseDelta();
    _aim += mouseDelta * (MouseSensitivity / AimRadius);
    const float aimLen = glm::length(_aim);
    if (aimLen > 1.0f) _aim /= aimLen;
    if (MouseReturn > 0.0f) _aim -= _aim * (1.0f - std::exp(-MouseReturn * dt));
    input->SetMouseCapture(true);

    glm::vec2 steer{0.0f};
    const float mag = glm::length(_aim);
    if (mag > AimDeadZone) steer = _aim * ((mag - AimDeadZone) / (1.0f - AimDeadZone) / mag);

    const float pitchSign = InvertPitch ? 1.0f : -1.0f;
    targetOmega.x += pitchSign * steer.y * PitchRate;
    targetOmega.y += steer.x * YawRate;
    if (input->GetKey(GLFW_KEY_A)) targetOmega.z += RollRate;
    if (input->GetKey(GLFW_KEY_D)) targetOmega.z -= RollRate;

    const float rotAlpha = 1.0f - std::exp(-RotationResponse * dt);
    const float rollResponse = std::abs(targetOmega.z) > std::abs(_angularVelocity.z) ? RollAccel : RollDecel;
    const float rollAlpha = 1.0f - std::exp(-rollResponse * dt);
    _angularVelocity.x += (targetOmega.x - _angularVelocity.x) * rotAlpha;
    _angularVelocity.y += (targetOmega.y - _angularVelocity.y) * rotAlpha;
    _angularVelocity.z += (targetOmega.z - _angularVelocity.z) * rollAlpha;
    _camera->RotateLocal(_angularVelocity.x * dt, _angularVelocity.y * dt, _angularVelocity.z * dt);

    const glm::quat orientation = _camera->GetOrientation();
    const glm::vec3 front = orientation * glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 thrust{0.0f};
    if (input->GetKey(GLFW_KEY_W)) thrust += front;
    if (input->GetKey(GLFW_KEY_S)) thrust -= front;
    if (input->GetKey(GLFW_KEY_Q)) thrust -= up;
    if (input->GetKey(GLFW_KEY_E)) thrust += up;

    float accel = ThrustAccel;
    if (input->GetKey(GLFW_KEY_LEFT_SHIFT)) accel *= BoostMultiplier;

    if (glm::length(thrust) > 0.0001f)
        _velocity += glm::normalize(thrust) * accel * dt;

    if (input->GetKeyDown(GLFW_KEY_SPACE)) FlightAssist = !FlightAssist;

    if (input->GetKey(GLFW_KEY_X)) {
        _velocity -= _velocity * (1.0f - std::exp(-FullStopDamping * dt));
    } else if (FlightAssist) {
        _velocity -= _velocity * (1.0f - std::exp(-FlightAssistDamping * dt));
    }

    const float speed = glm::length(_velocity);
    if (speed > MaxSpeed) _velocity *= MaxSpeed / speed;

    _camera->Position += _velocity * dt;
    _camera->Update();
}

auto ShipCameraController::DrawDebugUI() -> void {
    ImGui::Begin("Ship Camera");
    ImGui::Text("Flight Assist: %s  (Space toggles)", FlightAssist ? "ON" : "OFF");
    ImGui::Text("Speed: %.1f / %.0f", glm::length(_velocity), MaxSpeed);
    ImGui::Text("Velocity : %+.1f %+.1f %+.1f", _velocity.x, _velocity.y, _velocity.z);
    ImGui::Text("AngVel   : P%+.0f Y%+.0f R%+.0f", _angularVelocity.x, _angularVelocity.y, _angularVelocity.z);
    ImGui::Text("Aim      : %+.2f %+.2f", _aim.x, _aim.y);
    ImGui::TextUnformatted("Mouse = yaw/pitch, A/D = roll.");
    ImGui::TextUnformatted("W/S = thrust, Q/E = up/down, Shift = boost, X = stop.");
    ImGui::End();
}
