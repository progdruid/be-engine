#pragma once

#include <vector>
#include <algorithm>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

enum class BeTrackInterp { Hold, Linear, EaseIn, EaseOut, EaseInOut };

template<class T>
class BeTrack {
    hide
    struct Keyframe { float time; T value; BeTrackInterp out; };
    std::vector<Keyframe> _keys;

    expose
    auto Key(float time, T value, BeTrackInterp interp) -> BeTrack& {
        const Keyframe k{time, value, interp};
        const auto it = std::upper_bound(_keys.begin(), _keys.end(), time,
            [](float t, const Keyframe& a) { return t < a.time; });
        _keys.insert(it, k);
        return *this;
    }

    auto Eval(float time) const -> T {
        const Sample s = SampleAt(time);
        return glm::mix(s.from, s.to, s.k);
    }

    struct Sample { T from; T to; float k; };

    auto SampleAt(float time) const -> Sample {
        if (_keys.empty()) return { T{}, T{}, 0.0f };
        if (time <= _keys.front().time) return { _keys.front().value, _keys.front().value, 0.0f };
        if (time >= _keys.back().time)  return { _keys.back().value, _keys.back().value, 1.0f };

        size_t i = 0;
        while (i + 1 < _keys.size() && _keys[i + 1].time <= time) ++i;
        const Keyframe& a = _keys[i];
        const Keyframe& b = _keys[i + 1];
        const float span = b.time - a.time;
        const float f = span > 0.0f ? (time - a.time) / span : 0.0f;

        float e = f;
        switch (a.out) {
            case BeTrackInterp::Hold:      e = 0.0f; break;
            case BeTrackInterp::EaseIn:    e = f * f; break;
            case BeTrackInterp::EaseOut:   e = 1.0f - (1.0f - f) * (1.0f - f); break;
            case BeTrackInterp::EaseInOut: e = f * f * (3.0f - 2.0f * f); break;
            default:                       break;
        }
        return { a.value, b.value, e };
    }

    auto Empty() const -> bool { return _keys.empty(); }
    auto End() const -> float { return _keys.empty() ? 0.0f : _keys.back().time; }
};
