#include "RiftTerrain.h"

#include <cmath>
#include <cstdint>

#include <umbrellas/include-glm.h>

#include "BeMesh.h"

namespace {

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

namespace RiftTerrain {

auto SampleHeight(float worldX, float worldZ, float size, float spikeAmplitude) -> float {
    const float half = size * 0.5f;
    float u = (half - worldX) / size;
    float v = (worldZ + half) / size;
    u -= std::floor(u);
    v -= std::floor(v);
    return HeightAtUV(u, v, spikeAmplitude);
}

auto BuildMesh(float size, int cells, float spikeAmplitude) -> std::shared_ptr<BeMesh> {
    const float half = size * 0.5f;
    const float cell = size / cells;

    auto vpos = [&](int gx, int gz) -> glm::vec3 {
        float u = static_cast<float>(gx) / cells;
        float v = static_cast<float>(gz) / cells;
        return glm::vec3(-(gx * cell - half), HeightAtUV(u, v, spikeAmplitude), gz * cell - half);
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

    for (int gz = 0; gz < cells; ++gz) {
        for (int gx = 0; gx < cells; ++gx) {
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

}
