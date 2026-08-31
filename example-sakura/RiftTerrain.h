#pragma once

#include <memory>
#include <vector>

#include <umbrellas/include-glm.h>
#include <umbrellas/common.hpp>

class BeMesh;

class RiftTerrain {
    expose
    struct SurfaceHit {
        float Height;
        glm::vec3 Normal;
    };

    struct Collision {
        bool Hit = false;
        glm::vec3 Position{0.0f};
        glm::vec3 Normal{0.0f, 1.0f, 0.0f};
        float Penetration = 0.0f;
    };

    RiftTerrain();

    [[nodiscard]] auto GetSurface(float worldX, float worldZ) const -> SurfaceHit;
    [[nodiscard]] auto GetHeight(float worldX, float worldZ) const -> float;
    [[nodiscard]] auto CollideSphere(glm::vec3 center, float radius) const -> Collision;

    [[nodiscard]] auto GetMapSize() const -> float { return _size; }
    [[nodiscard]] auto GetResolution() const -> int { return _cells; }
    [[nodiscard]] auto CopyPackedHeights() const -> std::vector<float>;

    hide
    [[nodiscard]] auto GetVertexHeight(int i, int j) const -> float;

    float _size;
    float _half;
    float _cell;
    int _cells;
    std::vector<float> _heights;
};
