#include "DeliverySystem.h"

#include <cmath>
#include <cstdio>

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

auto DeliverySystem::GetDistanceToTarget(glm::vec3 shipPos) const -> float {
    be_assert(_contract, "GetDistanceToTarget: no active contract");
    return glm::length(_stations[_contract->Destination].Aim - shipPos);
}

auto DeliverySystem::GetStationName(int station) const -> std::string {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "Station %02d", station + 1);
    return buffer;
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

    if (_stations.size() <= 1) return;
    const int jobCount = config.JobsPerStation < 0 ? 0 : config.JobsPerStation;
    std::uniform_int_distribution<int> pickStation(0, static_cast<int>(_stations.size()) - 1);
    for (int index = 0; index < static_cast<int>(_stations.size()); ++index) {
        auto& station = _stations[index];
        station.Jobs.reserve(jobCount);
        for (int job = 0; job < jobCount; ++job) {
            int destination = index;
            while (destination == index) destination = pickStation(_rng);
            const float distance = glm::length(_stations[destination].Position - station.Position);
            const int reward = static_cast<int>(config.RewardBase + distance * config.RewardPerMeter);
            station.Jobs.push_back({ .Destination = destination, .Distance = distance, .Reward = reward });
        }
    }
}

auto DeliverySystem::CheckDock(glm::vec3 shipPos) const -> DockHit {
    for (int index = 0; index < static_cast<int>(_stations.size()); ++index) {
        const auto& station = _stations[index];
        for (const auto& dock : station.Docks) {
            if (glm::length(dock - shipPos) <= station.DockRadius) {
                return { .Hit = true, .Anchor = dock, .Station = index };
            }
        }
    }
    return {};
}

auto DeliverySystem::GetTargetPosition(glm::vec3 shipPos) const -> glm::vec3 {
    be_assert(_contract, "GetTargetPosition: no active contract");
    const auto& station = _stations[_contract->Destination];
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
    _dockedStation = hit.Station;
}

auto DeliverySystem::NotifyUndocked() -> void {
    _dockedStation = -1;
}

auto DeliverySystem::TakeJob(int station, int jobIndex) -> void {
    be_assert(station >= 0 && station < static_cast<int>(_stations.size()), "TakeJob: station out of range");
    const auto& jobs = _stations[station].Jobs;
    be_assert(jobIndex >= 0 && jobIndex < static_cast<int>(jobs.size()), "TakeJob: job out of range");
    _contract = jobs[jobIndex];
}

auto DeliverySystem::CompleteContract() -> void {
    be_assert(CanComplete(), "CompleteContract: not at contract destination");
    _credits += _contract->Reward;
    _contract.reset();
}
