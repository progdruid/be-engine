#pragma once

#include <vector>

#include <common.hpp>

#include "coroutine/BeCoroutine.h"

class BeCoroutineScheduler {
    hide
    std::vector<BeCoroutine> _active;

    expose
    auto Start(BeCoroutine coroutine) -> void;
    auto Update(float deltaTime) -> void;
    auto Clear() -> void;
    auto IsEmpty() const -> bool;
};
