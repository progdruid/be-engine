#pragma once

#include <string>
#include <unordered_map>

#include <umbrellas/access-modifiers.hpp>

#include "BeCameraShot.h"

class BeCamera;

class RigCameraController {
    hide
    BeCamera* _camera;

    std::unordered_map<std::string, BeCameraShot> _shots;
    const BeCameraShot* _current = nullptr;
    float _t = 0.0f;
    float _speed = 1.0f;
    bool _playing = false;
    bool _loop = false;

    expose
    float AimDamping = 0.0f;

    explicit RigCameraController(BeCamera* camera);

    auto AddShot(std::string name) -> BeCameraShot&;
    auto GetCurrentShot() const -> const BeCameraShot* { return _current; };
    auto Play(const std::string& name, bool loop = false, float speed = 1.0f) -> void;
    auto Pause() -> void;
    auto Resume() -> void;
    auto Stop() -> void;
    auto Seek(float seconds) -> void;
    auto SetSpeed(float speed) -> void { _speed = speed; }

    auto Update(float deltaTime) -> void;

    auto IsPlaying() const -> bool { return _playing; }
    auto IsFinished() const -> bool;
};
