#include "BeFileWatcher.h"

std::vector<BeFileWatcher::Watch> BeFileWatcher::_watches;
BeFileWatcher::WatchId BeFileWatcher::_nextId = 1;
bool BeFileWatcher::_enabled = true;
float BeFileWatcher::_accum = 0.f;

static constexpr float PollInterval = 0.25f;

auto BeFileWatcher::Register(PathProvider provider, ChangeHandler onChange) -> WatchId {
    const WatchId id = _nextId++;
    _watches.push_back({ id, std::move(provider), std::move(onChange), {} });
    return id;
}

auto BeFileWatcher::Unregister(WatchId id) -> void {
    std::erase_if(_watches, [id](const Watch& watch) -> bool { return watch.Id == id; });
}

auto BeFileWatcher::SetEnabled(bool enabled) -> void {
    _enabled = enabled;
    if (!enabled) {
        _accum = 0.f;
    }
}

auto BeFileWatcher::Poll(float deltaTime) -> void {
    if (!_enabled) {
        return;
    }
    _accum += deltaTime;
    if (_accum < PollInterval) {
        return;
    }
    _accum = 0.f;

    for (auto& watch : _watches) {
        const auto paths = watch.Provider();

        auto changed = std::vector<std::filesystem::path>();
        for (const auto& path : paths) {
            std::error_code ec;
            const auto time = std::filesystem::last_write_time(path, ec);
            if (ec) {
                continue;
            }
            const auto it = watch.Times.find(path);
            if (it == watch.Times.end()) {
                watch.Times.emplace(path, time);
            } else if (it->second != time) {
                it->second = time;
                changed.push_back(path);
            }
        }

        if (!changed.empty()) {
            watch.OnChange(changed);
        }
    }
}
