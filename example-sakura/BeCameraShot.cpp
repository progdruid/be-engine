#include "BeCameraShot.h"

#include <algorithm>
#include <cmath>

namespace {
    auto SlerpDir(const glm::vec3& a, const glm::vec3& b, float t) -> glm::vec3 {
        const float la = glm::length(a), lb = glm::length(b);
        if (la < 1e-6f) return lb < 1e-6f ? glm::vec3(0.0f) : b / lb;
        if (lb < 1e-6f) return a / la;
        const glm::vec3 na = a / la, nb = b / lb;
        const float theta = std::acos(glm::clamp(glm::dot(na, nb), -1.0f, 1.0f));
        if (theta < 1e-4f) return na;
        const float s = std::sin(theta);
        return (std::sin((1.0f - t) * theta) / s) * na + (std::sin(t * theta) / s) * nb;
    }
}

auto BeCameraShot::SetPathRail(BeRail path) -> BeCameraShot& {
    _pathRail = std::move(path);
    return *this;
}

auto BeCameraShot::Duration() const -> float {
    return std::max({_pathWarp.End(), _aimTarget.End(), _fov.End()});
}

auto BeCameraShot::Sample(float t) const -> BeCameraShotSample {
    BeCameraShotSample out;

    const float duration = Duration();
    const float posDist = _pathWarp.Empty()
        ? (duration > 0.0f ? glm::clamp(t / duration, 0.0f, 1.0f) : 0.0f)
        : glm::clamp(_pathWarp.Eval(t), 0.0f, 1.0f);
    out.Position = _pathRail.EvalByDistance(posDist);

    glm::vec3 dir(0.0f);
    if (_aimMode == BeCameraShotAim::Tangent) {
        dir = _pathRail.TangentByDistance(posDist);
    } else if (!_aimTarget.Empty()) {
        const auto s = _aimTarget.SampleAt(t);
        dir = SlerpDir(s.from - out.Position, s.to - out.Position, s.k);
    }
    out.LookDir = dir;

    if (!_fov.Empty()) {
        out.HasFov = true;
        out.Fov = _fov.Eval(t);
    }

    return out;
}
