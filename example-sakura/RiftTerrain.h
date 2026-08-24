#pragma once

#include <memory>

class BeMesh;

namespace RiftTerrain {

auto SampleHeight(float worldX, float worldZ, float size, float spikeAmplitude) -> float;
auto BuildMesh(float size, int cells, float spikeAmplitude) -> std::shared_ptr<BeMesh>;

}
