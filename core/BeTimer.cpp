#include "BeTimer.h"

#include <cstdio>
#include "umbrellas/include-libassert.h"

void BeTimer::Start(const char* label) {
    be_assert(!_running, "BeTimer::Start called while already running");
    _label = label;
    _lastTime = 0;
    _running = true;
    _start = std::chrono::high_resolution_clock::now();
}

void BeTimer::End() {
    be_assert(_running, "BeTimer::End called while not running");
    _lastTime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - _start).count();
    _running = false;
}

void BeTimer::PrintLast() const {
    std::printf("[%s] %lld us (%.3f ms)\n", _label.c_str(), (long long)_lastTime, _lastTime / 1000.0);
}
