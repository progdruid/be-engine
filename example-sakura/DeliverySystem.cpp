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
        _stations.push_back({ .Position = position, .Aim = pivot + kind.AimPoint * kind.Scale, .Entity = entity, .DockRadius = kind.DockRadius });
        auto& station = _stations.back();

        auto dockProp = _assets.GetProp("dock-ring").lock();
        be_assert(dockProp, "DeliverySystem: missing dock-ring prop");
        for (size_t dock = 0; dock < kind.DockPositions.size(); ++dock) {
            const glm::vec3 dockWorld = pivot + rotation * (kind.DockPositions[dock] * kind.Scale);
            station.Docks.push_back(dockWorld);
            CreateEntity(_registry
                ,NameComponent { .Name = "dock-" + std::to_string(index) + "-" + std::to_string(dock) }
                ,TransformComponent { .Position = dockWorld, .Scale = glm::vec3(kind.DockRadius) }
                ,RenderComponent { .Prop = dockProp, .CastShadows = false }
                ,DockComponent { .StationIndex = index }
            );
        }
    }
}

auto DeliverySystem::Begin(glm::vec3 shipPos) -> void {
    if (_stations.empty()) return;
    _target = PickTargetExcept(NearestStation(shipPos));
}

auto DeliverySystem::CheckDock(glm::vec3 shipPos) const -> DockHit {
    for (int index = 0; index < static_cast<int>(_stations.size()); ++index) {
        const auto& station = _stations[index];
        for (const auto& dock : station.Docks) {
            if (glm::length(dock - shipPos) <= station.DockRadius) {
                return { .Hit = true, .Anchor = dock, .IsTarget = index == _target };
            }
        }
    }
    return {};
}

auto DeliverySystem::TargetPosition(glm::vec3 shipPos) const -> glm::vec3 {
    const auto& station = _stations[_target];
    if (station.Docks.empty()) return station.Aim;

    glm::vec3 closest = station.Docks[0];
    float best = glm::dot(closest - shipPos, closest - shipPos);
    for (const auto& dock : station.Docks) {
        const glm::vec3 delta = dock - shipPos;
        const float distanceSq = glm::dot(delta, delta);
        if (distanceSq < best) {
            best = distanceSq;
            closest = dock;
        }
    }
    return closest;
}

auto DeliverySystem::NotifyDocked(const DockHit& hit) -> void {
    if (!hit.IsTarget || _target < 0) return;
    ++_delivered;
    _target = PickTargetExcept(_target);
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
