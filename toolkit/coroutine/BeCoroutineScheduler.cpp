#include "coroutine/BeCoroutineScheduler.h"

#include <algorithm>
#include <utility>

auto BeCoroutineScheduler::Start(BeCoroutine coroutine) -> void {
    _active.push_back(std::move(coroutine));
}

auto BeCoroutineScheduler::Update(float deltaTime) -> void {
    for (size_t i = 0; i < _active.size(); ++i) {
        const auto handle = _active[i].GetHandle();
        if (!handle || handle.done()) continue;
        auto& promise = handle.promise();
        promise.Wait -= deltaTime;
        if (promise.Wait <= 0.0f) {
            handle.resume();
        }
    }

    std::erase_if(_active, [](const BeCoroutine& coroutine) -> bool {
        const auto handle = coroutine.GetHandle();
        return !handle || handle.done();
    });
}

auto BeCoroutineScheduler::Clear() -> void {
    _active.clear();
}

auto BeCoroutineScheduler::IsEmpty() const -> bool {
    return _active.empty();
}
