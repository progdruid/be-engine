#pragma once

#include <vector>
#include <umbrellas/include-glm.h>
#include <umbrellas/access-modifiers.hpp>

class BeRail {
    hide
    std::vector<glm::vec3> _knots;
    std::vector<float> _arcLen;
    std::vector<float> _arcU;
    float _length = 0.0f;
    bool _closed = false;

    expose
    auto Knot(glm::vec3 p) -> BeRail&;
    auto Close() -> BeRail&;
    auto Finalize() -> BeRail&;

    auto EvalByDistance(float s01) const -> glm::vec3;
    auto TangentByDistance(float s01) const -> glm::vec3;
    auto EvalByIndex(float index) const -> glm::vec3;
    auto IndexToDistance(float index) const -> float;
    auto SegmentCount() const -> int;
    auto Length() const -> float { return _length; }
    auto KnotCount() const -> size_t { return _knots.size(); }

    hide
    auto EvalByParam(float u) const -> glm::vec3;
};
