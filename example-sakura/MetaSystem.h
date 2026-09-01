#pragma once

#include <umbrellas/common.hpp>

class DeliverySystem;

class MetaSystem {
    expose
    enum class Phase { Inactive, Briefing, Running, Won, Lost, Free };

    hide
    Phase _phase = Phase::Inactive;
    Phase _popupOpenedFor = Phase::Inactive;
    float _elapsed = 0.0f;
    bool _wantsClose = false;
    
    expose
    auto Begin() -> void;
    auto End() -> void;
    auto Update(float deltaTime, const DeliverySystem& delivery) -> void;
    auto DrawUI(const DeliverySystem& delivery) -> void;

    [[nodiscard]] auto IsPaused() const -> bool;
    [[nodiscard]] auto WantsClose() const -> bool { return _wantsClose; }
    [[nodiscard]] auto GetElapsed() const -> float { return _elapsed; }
    auto SetElapsed(float elapsed) -> void;
};
