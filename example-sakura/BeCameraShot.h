#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "BeRail.h"
#include "BeTrack.h"

enum class BeCameraShotAim { Target, Tangent };

struct BeCameraShotSample {
    glm::vec3 Position{0.0f};
    glm::vec3 LookDir{0.0f};
    bool HasFov = false;
    float Fov = 0.0f;
};

class BeCameraShot {
    hide
    BeRail _pathRail;
    BeTrack<float> _pathWarp;

    BeCameraShotAim _aimMode = BeCameraShotAim::Target;
    BeTrack<glm::vec3> _aimTarget;
    BeTrack<float> _fov;

    expose
    auto SetPathRail(BeRail path) -> BeCameraShot&;
    auto GetPathRail() const -> const BeRail& { return _pathRail; }
    auto PathWarp() -> BeTrack<float>& { return _pathWarp; }

    auto AimAt() -> BeTrack<glm::vec3>& { _aimMode = BeCameraShotAim::Target; return _aimTarget; }
    auto AimAlongTangent() -> BeCameraShot& { _aimMode = BeCameraShotAim::Tangent; return *this; }

    auto Fov() -> BeTrack<float>& { return _fov; }

    auto Duration() const -> float;
    auto Sample(float t) const -> BeCameraShotSample;
};
