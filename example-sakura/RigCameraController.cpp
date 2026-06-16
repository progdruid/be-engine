#include "RigCameraController.h"

#include <algorithm>
#include <cmath>

#include <umbrellas/include-libassert.h>

#include "BeCamera.h"
#include "BeCameraShot.h"

namespace {
    auto ShortestAngle(float from, float to) -> float {
        return std::fmod(to - from + 540.0f, 360.0f) - 180.0f;
    }
}

RigCameraController::RigCameraController(BeCamera* camera)
    : _camera(camera)
{}

auto RigCameraController::AddShot(std::string name) -> BeCameraShot& {
    return _shots[std::move(name)];
}

auto RigCameraController::Play(const std::string& name, bool loop, float speed) -> void {
    const auto it = _shots.find(name);
    if (it == _shots.end()) return;
    _current = &it->second;
    _loop = loop;
    _speed = speed;
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
    be_assert(_current);
    _t = glm::clamp(seconds, 0.0f, _current->Duration());
}

auto RigCameraController::IsFinished() const -> bool {
    be_assert(_current);
    return !_loop && _t >= _current->Duration();
}

auto RigCameraController::Update(float deltaTime) -> void {
    if (!_playing || !_current) return;

    const float duration = _current->Duration();
    if (duration <= 0.0f) return;

    _t += deltaTime * _speed;
    if (_t >= duration) {
        if (_loop) _t = std::fmod(_t, duration);
        else       { _t = duration; _playing = false; }
    }

    const BeCameraShotSample sample = _current->Sample(_t);

    const float len = glm::length(sample.LookDir);
    if (len > 1e-5f) {
        const glm::vec3 d = sample.LookDir / len;
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

    if (sample.HasFov) _camera->Fov = sample.Fov;

    _camera->Position = sample.Position;
    _camera->Update();
}
