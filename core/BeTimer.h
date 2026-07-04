#pragma once

#include <chrono>
#include <string>
#include <umbrellas/common.hpp>

class BeTimer {
    hide
    std::string _label{};
    std::chrono::high_resolution_clock::time_point _start{};
    int64_t _lastTime{0};
    bool _running{false};
    
    expose
    void Start(const char* label);
    void End();
    void PrintLast() const;
};
