#pragma once
#include <cstdint>
#include <umbrellas/common.hpp>

class FpsCounter {
    hide
    float _accumulated = 0.0f;
    uint32_t _frames = 0;
    float _fps = 0.0f;
    float _frameMs = 0.0f;

    expose
    static constexpr float RefreshInterval = 0.25f;

    auto Tick(float deltaTime) -> void {
        _accumulated += deltaTime;
        ++_frames;
        if (_accumulated < RefreshInterval) {
            return;
        }
        _fps = static_cast<float>(_frames) / _accumulated;
        _frameMs = 1000.0f * _accumulated / static_cast<float>(_frames);
        _accumulated = 0.0f;
        _frames = 0;
    }

    auto GetFps() const -> float { return _fps; }
    auto GetFrameMs() const -> float { return _frameMs; }
};
