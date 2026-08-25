#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

class BeCamera;
class BeInput;

class ShipCameraController {
    expose
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
