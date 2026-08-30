#include "RiftTerrain.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "BeMesh.h"

namespace {

auto ClosestPointOnTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) -> glm::vec3 {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 ap = p - a;
    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    const glm::vec3 bp = p - b;
    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) return a + ab * (d1 / (d1 - d3));

    const glm::vec3 cp = p - c;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) return a + ac * (d2 / (d2 - d6));

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));

    const float denom = 1.0f / (va + vb + vc);
    return a + ab * (vb * denom) + ac * (vc * denom);
}

auto Hash(int x, int z) -> float {
    uint32_t h = static_cast<uint32_t>(x * 374761393 + z * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFF) / 65535.0f;
}

auto SmootherStep(float t) -> float {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

auto ValueNoise(float u, float v, int freq) -> float {
    float px = u * freq;
    float pz = v * freq;
    int x0 = static_cast<int>(std::floor(px));
    int z0 = static_cast<int>(std::floor(pz));
    float tx = SmootherStep(px - x0);
    float tz = SmootherStep(pz - z0);
    int seed = freq * 131;
    int x1 = (x0 + 1) % freq;
    int z1 = (z0 + 1) % freq;
    x0 = x0 % freq;
    z0 = z0 % freq;
    float c00 = Hash(x0 + seed, z0);
    float c10 = Hash(x1 + seed, z0);
    float c01 = Hash(x0 + seed, z1);
    float c11 = Hash(x1 + seed, z1);
    float a = c00 + (c10 - c00) * tx;
    float b = c01 + (c11 - c01) * tx;
    return a + (b - a) * tz;
}

auto RidgedFbm(float u, float v) -> float {
    float sum = 0.0f;
    float amp = 1.0f;
    float norm = 0.0f;
    int freq = 2;
    for (int octave = 0; octave < 5; ++octave) {
        float n = ValueNoise(u, v, freq);
        float ridge = 1.0f - std::fabs(2.0f * n - 1.0f);
        ridge *= ridge;
        sum += amp * ridge;
        norm += amp;
        amp *= 0.55f;
        freq *= 2;
    }
    return sum / norm;
}

auto HeightAtUV(float u, float v, float spikeAmplitude) -> float {
    float e = RidgedFbm(u, v);
    return std::pow(e, 2.0f) * spikeAmplitude;
}

}

RiftTerrain::RiftTerrain(float size, int cells, float spikeAmplitude)
    : _size(size)
    , _half(size * 0.5f)
    , _cell(size / cells)
    , _cells(cells)
{
    const int stride = cells + 1;
    _heights.resize(static_cast<size_t>(stride) * stride);
    for (int j = 0; j <= cells; ++j) {
        for (int i = 0; i <= cells; ++i) {
            _heights[static_cast<size_t>(j) * stride + i] =
                HeightAtUV(static_cast<float>(i) / cells, static_cast<float>(j) / cells, spikeAmplitude);
        }
    }
}

auto RiftTerrain::GetVertexHeight(int i, int j) const -> float {
    return _heights[static_cast<size_t>(j) * (_cells + 1) + i];
}

auto RiftTerrain::BuildMesh() const -> std::shared_ptr<BeMesh> {
    auto vpos = [&](int gx, int gz) -> glm::vec3 {
        return glm::vec3(-(gx * _cell - _half), GetVertexHeight(gx, gz), gz * _cell - _half);
    };

    auto mesh = std::make_shared<BeMesh>();
    auto pushTri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        uint32_t base = static_cast<uint32_t>(mesh->Vertices.size());
        for (const auto& p : { a, b, c }) {
            BeFullVertex v{};
            v.Position = p;
            v.Normal = normal;
            mesh->Vertices.push_back(v);
        }
        mesh->Indices.push_back(base + 0);
        mesh->Indices.push_back(base + 1);
        mesh->Indices.push_back(base + 2);
    };

    for (int gz = 0; gz < _cells; ++gz) {
        for (int gx = 0; gx < _cells; ++gx) {
            glm::vec3 topLeft = vpos(gx, gz);
            glm::vec3 topRight = vpos(gx + 1, gz);
            glm::vec3 bottomLeft = vpos(gx, gz + 1);
            glm::vec3 bottomRight = vpos(gx + 1, gz + 1);
            pushTri(topLeft, topRight, bottomLeft);
            pushTri(topRight, bottomRight, bottomLeft);
        }
    }

    mesh->Slices.push_back({
        .IndexCount = static_cast<uint32_t>(mesh->Indices.size()),
        .StartIndexLocation = 0,
        .BaseVertexLocation = 0,
    });

    return mesh;
}

auto RiftTerrain::GetSurface(float worldX, float worldZ) const -> SurfaceHit {
    float u = (_half - worldX) / _size;
    float v = (worldZ + _half) / _size;
    u -= std::floor(u);
    v -= std::floor(v);

    const float gu = u * _cells;
    const float gv = v * _cells;
    int i0 = static_cast<int>(gu);
    int j0 = static_cast<int>(gv);
    if (i0 >= _cells) i0 = _cells - 1;
    if (j0 >= _cells) j0 = _cells - 1;
    const float tx = gu - i0;
    const float tz = gv - j0;

    const float h00 = GetVertexHeight(i0, j0);
    const float h10 = GetVertexHeight(i0 + 1, j0);
    const float h01 = GetVertexHeight(i0, j0 + 1);
    const float h11 = GetVertexHeight(i0 + 1, j0 + 1);

    if (tx + tz <= 1.0f) {
        return {
            h00 + tx * (h10 - h00) + tz * (h01 - h00),
            glm::normalize(glm::vec3(h10 - h00, _cell, -(h01 - h00))),
        };
    }
    return {
        h11 + (1.0f - tx) * (h01 - h11) + (1.0f - tz) * (h10 - h11),
        glm::normalize(glm::vec3(h11 - h01, _cell, -(h11 - h10))),
    };
}

auto RiftTerrain::GetHeight(float worldX, float worldZ) const -> float {
    return GetSurface(worldX, worldZ).Height;
}

auto RiftTerrain::CollideSphere(glm::vec3 center, float radius) const -> Collision {
    auto wrap = [&](int i) { return ((i % _cells) + _cells) % _cells; };
    auto vertex = [&](int gi, int gj) -> glm::vec3 {
        return glm::vec3(
            _half - gi * _cell,
            _heights[static_cast<size_t>(wrap(gj)) * (_cells + 1) + wrap(gi)],
            gj * _cell - _half
        );
    };

    const int giMin = static_cast<int>(std::floor((_half - (center.x + radius)) / _cell));
    const int giMax = static_cast<int>(std::floor((_half - (center.x - radius)) / _cell));
    const int gjMin = static_cast<int>(std::floor((center.z - radius + _half) / _cell));
    const int gjMax = static_cast<int>(std::floor((center.z + radius + _half) / _cell));

    const float radiusSq = radius * radius;
    Collision best;

    auto consider = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        const glm::vec3 v0 = b - a;
        const glm::vec3 v1 = c - a;
        const glm::vec3 faceNormal = glm::normalize(glm::cross(v0, v1));
        const float signedDist = glm::dot(center - a, faceNormal);
        if (signedDist >= radius) return;

        const glm::vec3 rel = center - a;
        const float d00 = glm::dot(v0, v0);
        const float d01 = glm::dot(v0, v1);
        const float d11 = glm::dot(v1, v1);
        const float d20 = glm::dot(rel, v0);
        const float d21 = glm::dot(rel, v1);
        const float denom = d00 * d11 - d01 * d01;
        const float bary1 = (d11 * d20 - d01 * d21) / denom;
        const float bary2 = (d00 * d21 - d01 * d20) / denom;

        glm::vec3 pushNormal;
        float penetration;
        if (bary1 >= 0.0f && bary2 >= 0.0f && bary1 + bary2 <= 1.0f) {
            pushNormal = faceNormal;
            penetration = radius - signedDist;
        } else {
            const glm::vec3 closest = ClosestPointOnTriangle(center, a, b, c);
            const glm::vec3 delta = center - closest;
            const float distSq = glm::dot(delta, delta);
            if (distSq >= radiusSq) return;
            const float dist = std::sqrt(distSq);
            penetration = radius - dist;
            pushNormal = dist > 1e-5f ? delta / dist : faceNormal;
        }

        if (best.Hit && penetration <= best.Penetration) return;
        best = { true, center + pushNormal * penetration, pushNormal, penetration };
    };

    for (int gj = gjMin; gj <= gjMax; ++gj) {
        for (int gi = giMin; gi <= giMax; ++gi) {
            const glm::vec3 v00 = vertex(gi, gj);
            const glm::vec3 v10 = vertex(gi + 1, gj);
            const glm::vec3 v01 = vertex(gi, gj + 1);
            const glm::vec3 v11 = vertex(gi + 1, gj + 1);
            consider(v00, v10, v01);
            consider(v10, v11, v01);
        }
    }

    return best;
}
