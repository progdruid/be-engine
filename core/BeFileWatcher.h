#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <span>
#include <vector>

#include <umbrellas/common.hpp>

class BeFileWatcher {

    expose
    using WatchId = uint32_t;
    using PathProvider = std::function<std::vector<std::filesystem::path>()>;
    using ChangeHandler = std::function<void(std::span<const std::filesystem::path>)>;

    hide
    struct Watch {
        WatchId Id;
        PathProvider Provider;
        ChangeHandler OnChange;
        std::map<std::filesystem::path, std::filesystem::file_time_type> Times; // std::filesystem::path has no hasher
    };

    expose
    static auto Register(PathProvider provider, ChangeHandler onChange) -> WatchId;
    static auto Unregister(WatchId id) -> void;

    static auto Poll(float deltaTime) -> void;

    static auto SetEnabled(bool enabled) -> void;
    static auto IsEnabled() -> bool { return _enabled; }

    hide
    static std::vector<Watch> _watches;
    static WatchId _nextId;
    static bool _enabled;
    static float _accum;
};
