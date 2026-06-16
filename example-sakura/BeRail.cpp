#include "BeRail.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int kSamplesPerSegment = 32;

    auto CatmullRom(const glm::vec3& p0, const glm::vec3& p1,
                    const glm::vec3& p2, const glm::vec3& p3, float t) -> glm::vec3 {
        constexpr float alpha = 0.5f;
        const float dt0 = std::max(std::pow(glm::length(p1 - p0), alpha), 1e-4f);
        const float dt1 = std::max(std::pow(glm::length(p2 - p1), alpha), 1e-4f);
        const float dt2 = std::max(std::pow(glm::length(p3 - p2), alpha), 1e-4f);

        const float t0 = 0.0f;
        const float t1 = t0 + dt0;
        const float t2 = t1 + dt1;
        const float t3 = t2 + dt2;
        const float tt = t1 + t * (t2 - t1);   // local t maps into [t1, t2]

        // Barry-Goldman pyramidal evaluation; clamped dt's keep denominators > 0.
        const glm::vec3 a1 = (t1 - tt) / (t1 - t0) * p0 + (tt - t0) / (t1 - t0) * p1;
        const glm::vec3 a2 = (t2 - tt) / (t2 - t1) * p1 + (tt - t1) / (t2 - t1) * p2;
        const glm::vec3 a3 = (t3 - tt) / (t3 - t2) * p2 + (tt - t2) / (t3 - t2) * p3;
        const glm::vec3 b1 = (t2 - tt) / (t2 - t0) * a1 + (tt - t0) / (t2 - t0) * a2;
        const glm::vec3 b2 = (t3 - tt) / (t3 - t1) * a2 + (tt - t1) / (t3 - t1) * a3;
        return (t2 - tt) / (t2 - t1) * b1 + (tt - t1) / (t2 - t1) * b2;
    }
}

auto BeRail::Knot(glm::vec3 p) -> BeRail& {
    _knots.push_back(p);
    return *this;
}

auto BeRail::Close() -> BeRail& {
    _closed = true;
    return *this;
}

auto BeRail::Finalize() -> BeRail& {
    _arcLen.clear();
    _arcU.clear();
    _length = 0.0f;

    if (_knots.size() < 2) {
        _arcLen.push_back(0.0f);
        _arcU.push_back(0.0f);
        return *this;
    }

    const int segments = static_cast<int>(_knots.size()) - (_closed ? 0 : 1);
    const int total = segments * kSamplesPerSegment;

    glm::vec3 prev = EvalByParam(0.0f);
    _arcLen.push_back(0.0f);
    _arcU.push_back(0.0f);
    for (int i = 1; i <= total; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(total);
        const glm::vec3 cur = EvalByParam(u);
        _length += glm::length(cur - prev);
        _arcLen.push_back(_length);
        _arcU.push_back(u);
        prev = cur;
    }
    return *this;
}

auto BeRail::EvalByDistance(float s01) const -> glm::vec3 {
    if (_knots.empty()) return glm::vec3(0.0f);
    if (_knots.size() == 1 || _length <= 0.0f) return _knots.front();

    const float target = glm::clamp(s01, 0.0f, 1.0f) * _length;

    // Binary search the cumulative-length LUT, then lerp the param between samples.
    const auto it = std::lower_bound(_arcLen.begin(), _arcLen.end(), target);
    const size_t hi = std::min<size_t>(it - _arcLen.begin(), _arcLen.size() - 1);
    if (hi == 0) return EvalByParam(_arcU.front());

    const size_t lo = hi - 1;
    const float segLen = _arcLen[hi] - _arcLen[lo];
    const float f = segLen > 0.0f ? (target - _arcLen[lo]) / segLen : 0.0f;
    const float u = glm::mix(_arcU[lo], _arcU[hi], f);
    return EvalByParam(u);
}

auto BeRail::TangentByDistance(float s01) const -> glm::vec3 {
    constexpr float eps = 1e-3f;
    const glm::vec3 a = EvalByDistance(glm::clamp(s01 - eps, 0.0f, 1.0f));
    const glm::vec3 b = EvalByDistance(glm::clamp(s01 + eps, 0.0f, 1.0f));
    const glm::vec3 d = b - a;
    const float len = glm::length(d);
    return len > 1e-6f ? d / len : glm::vec3(0.0f, 0.0f, 1.0f);
}

auto BeRail::IndexToDistance(float index) const -> float {
    const int segments = SegmentCount();
    if (segments <= 0 || _length <= 0.0f || _arcLen.size() < 2) return 0.0f;

    // _arcLen/_arcU are sampled uniformly in param, so param maps straight to a LUT slot.
    const float u = glm::clamp(index / static_cast<float>(segments), 0.0f, 1.0f);
    const float fpos = u * static_cast<float>(_arcLen.size() - 1);
    const int lo = static_cast<int>(fpos);
    const int hi = std::min(lo + 1, static_cast<int>(_arcLen.size()) - 1);
    const float len = glm::mix(_arcLen[lo], _arcLen[hi], fpos - static_cast<float>(lo));
    return len / _length;
}

auto BeRail::SegmentCount() const -> int {
    const int n = static_cast<int>(_knots.size());
    if (n < 2) return 0;
    return _closed ? n : n - 1;
}

auto BeRail::EvalByIndex(float index) const -> glm::vec3 {
    const size_t n = _knots.size();
    if (n == 0) return glm::vec3(0.0f);
    if (n == 1) return _knots[0];

    const int count = static_cast<int>(n);
    const int segments = _closed ? count : count - 1;
    const float scaled = glm::clamp(index, 0.0f, static_cast<float>(segments));
    const int seg = std::min(static_cast<int>(scaled), segments - 1);
    const float localT = scaled - static_cast<float>(seg);

    // Closed paths wrap neighbours around; open paths clamp so endpoints duplicate.
    const auto at = [&](int i) -> const glm::vec3& {
        return _closed ? _knots[((i % count) + count) % count]
                       : _knots[glm::clamp(i, 0, count - 1)];
    };
    return CatmullRom(at(seg - 1), at(seg), at(seg + 1), at(seg + 2), localT);
}

auto BeRail::EvalByParam(float u) const -> glm::vec3 {
    return EvalByIndex(glm::clamp(u, 0.0f, 1.0f) * static_cast<float>(SegmentCount()));
}
