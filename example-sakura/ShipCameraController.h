#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

class BeCamera;
class BeInput;
class RiftTerrain;

class ShipCameraController {
    expose
    explicit ShipCameraController(BeCamera* camera, const RiftTerrain* terrain);

    auto Update(float deltaTime, BeInput* input) -> void;
    auto DrawDebugUI() -> void;
    auto Respawn(glm::vec3 position) -> void;
    [[nodiscard]] auto GetAim() const -> glm::vec2 { return _aim; }
    [[nodiscard]] auto GetLastImpactSpeed() const -> float { return _lastImpactSpeed; }

    auto SetInDock(bool inDock) -> void { _wasInDockLast = _isInDock; _isInDock = inDock; }
    [[nodiscard]] auto HasJustEnteredDock() const -> bool { return _isInDock && !_wasInDockLast; }

    auto Capture(glm::vec3 anchor) -> void { _isCaptured = true; _anchor = anchor; }
    auto Uncapture() -> void { _isCaptured = false; }
    [[nodiscard]] auto IsCaptured() const -> bool { return _isCaptured; }

    auto SetControlsEnabled(bool enabled) -> void { _controlsEnabled = enabled; }

    hide
    BeCamera* _camera;
    const RiftTerrain* _terrain;
    glm::vec3 _velocity{0.0f};
    glm::vec3 _angularVelocity{0.0f};
    glm::vec2 _aim{0.0f};
    float _lastImpactSpeed{0.0f};
    float _groundEffectProximity{0.0f};
    bool _isInDock{false};
    bool _wasInDockLast{false};
    bool _isCaptured{false};
    bool _controlsEnabled{true};
    glm::vec3 _anchor{0.0f};
};
