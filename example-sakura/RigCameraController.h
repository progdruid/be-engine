#pragma once

#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

#include "BeRail.h"
#include "BeTrack.h"

class BeCamera;

class RigCameraController {
    hide
    BeCamera* _camera;

    BeRail _pathRail;
    BeTrack<float> _pathWarp;
    BeRail _aimRail;
    BeTrack<float> _aimWarp;
    BeTrack<float> _fov;

    float _t = 0.0f;
    float _speed = 1.0f;
    bool _playing = false;
    bool _loop = false;

    expose
    explicit RigCameraController(BeCamera* camera);

    auto SetPathRail(BeRail path) -> void;
    auto GetPathRail() const -> const BeRail& { return _pathRail; }
    auto SetAimRail(BeRail path) -> void;
    auto GetAimRail() const -> const BeRail& { return _aimRail; }
    auto PathWarp() -> BeTrack<float>& { return _pathWarp; }
    auto AimWarp() -> BeTrack<float>& { return _aimWarp; }
    auto Fov() -> BeTrack<float>& { return _fov; }
    auto Configure(bool loop = false, float speed = 1.0f) -> void;

    auto Play() -> void;
    auto Pause() -> void;
    auto Resume() -> void;
    auto Stop() -> void;
    auto Seek(float seconds) -> void;
    auto SetSpeed(float speed) -> void { _speed = speed; }

    auto Update(float deltaTime) -> void;

    auto Duration() const -> float;
    auto IsPlaying() const -> bool { return _playing; }
    auto IsFinished() const -> bool;
};
