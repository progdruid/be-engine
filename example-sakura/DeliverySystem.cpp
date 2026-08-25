#include "DeliverySystem.h"

#include <cmath>
#include <limits>

#include "BeAssetRegistry.h"
#include "BeProp.h"
#include "Components.h"
#include "RiftSettings.h"
#include "RiftTerrain.h"

DeliverySystem::DeliverySystem(entt::registry& registry, BeAssetRegistry& assets)
    : _registry(registry)
    , _assets(assets)
    , _rng(RiftStore::Get().Delivery.Seed)
{}

DeliverySystem::~DeliverySystem() = default;

auto DeliverySystem::DistanceToTarget(glm::vec3 shipPos) const -> float {
    if (_target < 0) return std::numeric_limits<float>::max();
    return glm::length(_stations[_target].Aim - shipPos);
}

auto DeliverySystem::GenerateStations() -> void {
    const auto& config = RiftStore::Get().Delivery;
    const auto& terrain = RiftStore::Get().Terrain;
    if (config.Kinds.empty() || config.StationCount <= 0) return;

    std::uniform_int_distribution<size_t> pickKind(0, config.Kinds.size() - 1);
    _stations.reserve(config.StationCount);

    for (int index = 0; index < config.StationCount; ++index) {
        const auto& kind = config.Kinds[pickKind(_rng)];
        auto prop = _assets.GetProp(kind.Prop).lock();
        be_assert(prop, "DeliverySystem: missing station prop");

        glm::vec3 position{0.0f};
        for (int attempt = 0; attempt < 64; ++attempt) {
            const float radius = config.MapRadius * std::sqrt(FloatRange(0.0f, 1.0f).Pick(_rng));
            const float angle = FloatRange(0.0f, glm::two_pi<float>()).Pick(_rng);
            const float x = radius * std::cos(angle);
            const float z = radius * std::sin(angle);
            const float ground = RiftTerrain::SampleHeight(x, z, terrain.Size, terrain.SpikeAmplitude);
            const float y = kind.Flying ? ground + kind.AltitudeRange.Pick(_rng) : ground;
            position = glm::vec3(x, y, z);

            bool tooClose = false;
            for (const auto& station : _stations) {
                if (glm::length(station.Position - position) < config.MinSeparation) {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) break;
        }

        const glm::vec3 pivot = position - kind.Origin * kind.Scale;
        const glm::quat rotation = glm::quat(glm::vec3(
            kind.RotationX.Pick(_rng),
            kind.RotationY.Pick(_rng),
            kind.RotationZ.Pick(_rng)
        ));
        const auto entity = CreateEntity(_registry
            ,NameComponent { .Name = "station-" + std::to_string(index) }
            ,TransformComponent { .Position = pivot, .Rotation = rotation, .Scale = glm::vec3(kind.Scale) }
            ,RenderComponent { .Prop = prop, .CastShadows = false }
            ,StationComponent { .Index = index }
        );
        _stations.push_back({ .Position = position, .Aim = pivot + kind.AimPoint * kind.Scale, .Entity = entity });
    }
}

auto DeliverySystem::Begin(glm::vec3 shipPos) -> void {
    if (_stations.empty()) return;
    _target = PickTargetExcept(NearestStation(shipPos));
}

auto DeliverySystem::Update(glm::vec3 shipPos) -> bool {
    if (_target < 0) return false;
    if (glm::length(_stations[_target].Aim - shipPos) > RiftStore::Get().Delivery.VisitRadius) return false;

    ++_delivered;
    _target = PickTargetExcept(_target);
    return true;
}

auto DeliverySystem::TargetNearest(glm::vec3 shipPos) -> void {
    if (_stations.empty()) return;
    _target = NearestStation(shipPos);
}

auto DeliverySystem::NearestStation(glm::vec3 pos) const -> int {
    int nearest = -1;
    float best = std::numeric_limits<float>::max();
    for (int index = 0; index < static_cast<int>(_stations.size()); ++index) {
        const float distance = glm::length(_stations[index].Position - pos);
        if (distance < best) {
            best = distance;
            nearest = index;
        }
    }
    return nearest;
}

auto DeliverySystem::PickTargetExcept(int except) -> int {
    if (_stations.size() <= 1) return 0;
    std::uniform_int_distribution<int> pick(0, static_cast<int>(_stations.size()) - 1);
    int chosen = except;
    while (chosen == except) chosen = pick(_rng);
    return chosen;
}
