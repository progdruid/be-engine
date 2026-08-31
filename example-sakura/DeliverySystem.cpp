#include "DeliverySystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

#include "BeAssetRegistry.h"
#include "BeProp.h"
#include "Components.h"
#include "RiftSettings.h"
#include "RiftTerrain.h"

DeliverySystem::DeliverySystem(entt::registry& registry, BeAssetRegistry& assets, const RiftTerrain& terrain)
    : _registry(registry)
    , _assets(assets)
    , _terrain(terrain)
    , _rng(RiftStore::Get().Seed)
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
            const float ground = _terrain.GetHeight(x, z);
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

    const int commodityCount = static_cast<int>(config.Commodities.size());
    _cargo.assign(commodityCount, 0);
    if (commodityCount > 0) {
        const int minCount = std::clamp(config.MarketMinCommodities, 0, commodityCount);
        const int maxCount = std::clamp(config.MarketMaxCommodities, minCount, commodityCount);
        std::uniform_int_distribution<int> pickForSaleCount(minCount, maxCount);
        for (auto& station : _stations) {
            station.Market.reserve(commodityCount);
            for (int commodity = 0; commodity < commodityCount; ++commodity) {
                const float average = config.Commodities[commodity].AverageValue;
                const float factor = 1.0f + FloatRange(-config.PriceDeviation, config.PriceDeviation).Pick(_rng);
                const int price = std::max(1, static_cast<int>(std::lround(average * factor)));
                station.Market.push_back({ .Commodity = commodity, .Price = price, .ForSale = false });
            }

            std::vector<int> pool(commodityCount);
            std::iota(pool.begin(), pool.end(), 0);
            std::shuffle(pool.begin(), pool.end(), _rng);
            const int forSaleCount = pickForSaleCount(_rng);
            for (int slot = 0; slot < forSaleCount; ++slot) station.Market[pool[slot]].ForSale = true;
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

auto DeliverySystem::ApplyCrashPenalty() -> void {
    _credits /= 2;
    _contract.reset();
}

auto DeliverySystem::BuyCommodity(int station, int commodity, int tons) -> void {
    be_assert(station >= 0 && station < static_cast<int>(_stations.size()), "BuyCommodity: station out of range");
    be_assert(commodity >= 0 && commodity < static_cast<int>(_cargo.size()), "BuyCommodity: commodity out of range");
    be_assert(tons > 0, "BuyCommodity: non-positive tons");

    const auto& entry = _stations[station].Market[commodity];
    be_assert(entry.ForSale, "BuyCommodity: commodity not sold here");
    const int cost = entry.Price * tons;
    be_assert(cost <= _credits, "BuyCommodity: insufficient credits");
    be_assert(GetCargoUsed() + tons <= GetCargoCapacity(), "BuyCommodity: exceeds cargo capacity");

    _credits -= cost;
    _cargo[commodity] += tons;
}

auto DeliverySystem::SellCommodity(int station, int commodity, int tons) -> void {
    be_assert(station >= 0 && station < static_cast<int>(_stations.size()), "SellCommodity: station out of range");
    be_assert(commodity >= 0 && commodity < static_cast<int>(_cargo.size()), "SellCommodity: commodity out of range");
    be_assert(tons > 0, "SellCommodity: non-positive tons");
    be_assert(_cargo[commodity] >= tons, "SellCommodity: not enough cargo");

    _credits += _stations[station].Market[commodity].Price * tons;
    _cargo[commodity] -= tons;
}

auto DeliverySystem::GetCommodityName(int commodity) const -> std::string {
    const auto& commodities = RiftStore::Get().Delivery.Commodities;
    be_assert(commodity >= 0 && commodity < static_cast<int>(commodities.size()), "GetCommodityName: out of range");
    return commodities[commodity].Name;
}

auto DeliverySystem::GetCommodityAverage(int commodity) const -> float {
    const auto& commodities = RiftStore::Get().Delivery.Commodities;
    be_assert(commodity >= 0 && commodity < static_cast<int>(commodities.size()), "GetCommodityAverage: out of range");
    return commodities[commodity].AverageValue;
}

auto DeliverySystem::GetCargoUsed() const -> int {
    return std::accumulate(_cargo.begin(), _cargo.end(), 0);
}

auto DeliverySystem::GetCargoCapacity() const -> int {
    return RiftStore::Get().Delivery.CargoCapacity;
}

auto DeliverySystem::GetRespawnDock(glm::vec3 shipPos) -> glm::vec3 {
    be_assert(!_stations.empty(), "GetRespawnDock: no stations");

    const Station* closest = &_stations.front();
    float closestDistSq = glm::dot(shipPos - closest->Position, shipPos - closest->Position);
    for (const auto& station : _stations) {
        const float distSq = glm::dot(shipPos - station.Position, shipPos - station.Position);
        if (distSq < closestDistSq) {
            closest = &station;
            closestDistSq = distSq;
        }
    }

    if (closest->Docks.empty()) return closest->Position;
    std::uniform_int_distribution<size_t> pickDock(0, closest->Docks.size() - 1);
    return closest->Docks[pickDock(_rng)];
}
