#include "RigCameraController.h"

#include <algorithm>
#include <cmath>

#include "BeCamera.h"

RigCameraController::RigCameraController(BeCamera* camera)
    : _camera(camera)
{}

auto RigCameraController::SetPathRail(BeRail path) -> void {
    _pathRail = std::move(path);
}

auto RigCameraController::SetAimRail(BeRail path) -> void {
    _aimRail = std::move(path);
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
    return std::max({_pathWarp.End(), _aimWarp.End(), _fov.End()});
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

    if (_aimRail.KnotCount() > 0) {
        // Aim paces by its own warp, else shares the position progress.
        const float aimDist = _aimWarp.Empty() ? posDist
                                               : glm::clamp(_aimWarp.Eval(_t), 0.0f, 1.0f);
        const glm::vec3 dir = _aimRail.EvalByDistance(aimDist) - pos;
        const float len = glm::length(dir);
        if (len > 1e-5f) {
            const glm::vec3 d = dir / len;
            // Matches BeCamera's basis: front = (cy*cp, sp, sy*cp).
            _camera->Yaw   = glm::degrees(std::atan2(d.z, d.x));
            _camera->Pitch = glm::degrees(std::asin(glm::clamp(d.y, -1.0f, 1.0f)));
        }
    }

    if (!_fov.Empty()) _camera->Fov = _fov.Eval(_t);

    _camera->Position = pos;
    _camera->Update();
}
