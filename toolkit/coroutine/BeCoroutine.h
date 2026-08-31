#pragma once

#include <coroutine>
#include <exception>

#include <umbrellas/common.hpp>

class BeCoroutine {
    expose
    struct promise_type {
        float Wait = 0.0f;

        auto get_return_object() -> BeCoroutine {
            return BeCoroutine{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        auto final_suspend() noexcept -> std::suspend_always { return {}; }
        auto yield_value(float seconds) noexcept -> std::suspend_always {
            Wait = seconds;
            return {};
        }
        auto return_void() noexcept -> void {}
        auto unhandled_exception() -> void { std::terminate(); }
    };

    using Handle = std::coroutine_handle<promise_type>;

    
    hide
    Handle _handle;
    
    expose
    BeCoroutine() = default;
    explicit BeCoroutine(Handle handle) : _handle(handle) {}
    BeCoroutine(const BeCoroutine&) = delete;
    auto operator=(const BeCoroutine&) -> BeCoroutine& = delete;
    
    BeCoroutine(BeCoroutine&& other) noexcept : _handle(other._handle) {
        other._handle = {};
    }
    auto operator=(BeCoroutine&& other) noexcept -> BeCoroutine& {
        if (this != &other) {
            if (_handle) _handle.destroy();
            _handle = other._handle;
            other._handle = {};
        }
        return *this;
    }
    ~BeCoroutine() {
        if (_handle) _handle.destroy();
    }

    auto GetHandle() const -> Handle { return _handle; }
};
