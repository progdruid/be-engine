#pragma once

#include <vector>
#include <algorithm>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

enum class BeTrackInterp { Hold, Linear, EaseIn, EaseOut, EaseInOut, Smooth };

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
        if (_keys.empty()) return T{};
        if (time <= _keys.front().time) return _keys.front().value;
        if (time >= _keys.back().time)  return _keys.back().value;

        size_t i = 0;
        while (i + 1 < _keys.size() && _keys[i + 1].time <= time) ++i;
        const Keyframe& a = _keys[i];
        const Keyframe& b = _keys[i + 1];
        const float span = b.time - a.time;
        const float f = span > 0.0f ? (time - a.time) / span : 0.0f;

        if (a.out == BeTrackInterp::Smooth) {
            // Time-parameterised (non-uniform) Catmull-Rom via Hermite: tangents are
            // scaled by the surrounding time spacing, so distant keys don't yank the
            // curve and the result stays C1-continuous in time. Ends are one-sided.
            const Keyframe& p0 = _keys[i > 0 ? i - 1 : i];
            const Keyframe& p3 = _keys[i + 2 < _keys.size() ? i + 2 : i + 1];

            const T m1 = (b.value - p0.value) / std::max(b.time - p0.time, 1e-6f);  // tangent at a
            const T m2 = (p3.value - a.value) / std::max(p3.time - a.time, 1e-6f);  // tangent at b

            const float f2 = f * f, f3 = f2 * f;
            return (2.0f * f3 - 3.0f * f2 + 1.0f) * a.value
                 + (f3 - 2.0f * f2 + f) * (span * m1)
                 + (-2.0f * f3 + 3.0f * f2) * b.value
                 + (f3 - f2) * (span * m2);
        }

        float e = f;   // Linear
        switch (a.out) {
            case BeTrackInterp::Hold:      e = 0.0f; break;
            case BeTrackInterp::EaseIn:    e = f * f; break;
            case BeTrackInterp::EaseOut:   e = 1.0f - (1.0f - f) * (1.0f - f); break;
            case BeTrackInterp::EaseInOut: e = f * f * (3.0f - 2.0f * f); break;
            default:                       break;   // Linear / Smooth (handled above)
        }
        return glm::mix(a.value, b.value, e);
    }

    auto Empty() const -> bool { return _keys.empty(); }
    auto End() const -> float { return _keys.empty() ? 0.0f : _keys.back().time; }
};
