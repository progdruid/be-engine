#include "RigCameraController.h"

#include <algorithm>
#include <cmath>

#include "BeCamera.h"

namespace {
    auto ShortestAngle(float from, float to) -> float {
        return std::fmod(to - from + 540.0f, 360.0f) - 180.0f;
    }

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

RigCameraController::RigCameraController(BeCamera* camera)
    : _camera(camera)
{}

auto RigCameraController::SetPathRail(BeRail path) -> void {
    _pathRail = std::move(path);
}

auto RigCameraController::Configure(bool loop, float speed) -> void {
    _loop = loop;
    _speed = speed;
}

auto RigCameraController::Play() -> void {
    _t = 0.0f;
    _playing = true;
}

auto RigCameraController::Pause() -> void {
    _playing = false;
}

auto RigCameraController::Resume() -> void {
    _playing = true;
}

auto RigCameraController::Stop() -> void {
    _playing = false;
    _t = 0.0f;
}

auto RigCameraController::Seek(float seconds) -> void {
    _t = glm::clamp(seconds, 0.0f, Duration());
}

auto RigCameraController::Duration() const -> float {
    return std::max({_pathWarp.End(), _aimTarget.End(), _fov.End()});
}

auto RigCameraController::IsFinished() const -> bool {
    return !_loop && _t >= Duration();
}

auto RigCameraController::Update(float deltaTime) -> void {
    if (!_playing) return;

    const float duration = Duration();
    if (duration <= 0.0f) return;

    _t += deltaTime * _speed;
    if (_t >= duration) {
        if (_loop) _t = std::fmod(_t, duration);
        else       { _t = duration; _playing = false; }
    }

    // Pace along the path by arc-length fraction (uniform speed): the timewarp
    // track if authored, else sweep the whole path once over the duration.
    const float posDist = _pathWarp.Empty() ? (_t / duration)
                                            : glm::clamp(_pathWarp.Eval(_t), 0.0f, 1.0f);
    const glm::vec3 pos = _pathRail.EvalByDistance(posDist);

    glm::vec3 dir(0.0f);
    if (_aimMode == RigAim::Tangent) {
        dir = _pathRail.TangentByDistance(posDist);
    } else if (!_aimTarget.Empty()) {
        const auto s = _aimTarget.SampleAt(_t);
        dir = SlerpDir(s.from - pos, s.to - pos, s.k);
    }

    const float len = glm::length(dir);
    if (len > 1e-5f) {
        const glm::vec3 d = dir / len;
        // BeCamera basis: front = (cy*cp, sp, sy*cp).
        const float yaw   = glm::degrees(std::atan2(d.z, d.x));
        const float pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.0f, 1.0f)));
        if (AimDamping > 0.0f) {
            const float alpha = 1.0f - std::exp(-AimDamping * deltaTime);
            _camera->Yaw   += ShortestAngle(_camera->Yaw, yaw) * alpha;
            _camera->Pitch += (pitch - _camera->Pitch) * alpha;
        } else {
            _camera->Yaw   = yaw;
            _camera->Pitch = pitch;
        }
    }

    if (!_fov.Empty()) _camera->Fov = _fov.Eval(_t);

    _camera->Position = pos;
    _camera->Update();
}
