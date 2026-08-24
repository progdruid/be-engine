#include "DeliverySystem.h"

#include <cmath>
#include <limits>
#include <utility>

#include "BeAssetRegistry.h"
#include "BeProp.h"
#include "Components.h"

DeliverySystem::DeliverySystem(entt::registry& registry, BeAssetRegistry& assets, Config config)
    : _registry(registry)
    , _assets(assets)
    , _config(std::move(config))
    , _rng(_config.Seed)
{}

DeliverySystem::~DeliverySystem() = default;

auto DeliverySystem::RandomFloat(float lo, float hi) -> float {
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(_rng);
}

auto DeliverySystem::DistanceToTarget(glm::vec3 shipPos) const -> float {
    if (_target < 0) return std::numeric_limits<float>::max();
    return glm::length(_stations[_target].Position - shipPos);
}

auto DeliverySystem::GenerateStations() -> void {
    if (_config.StationProps.empty() || _config.StationCount <= 0) return;

    std::uniform_int_distribution<size_t> pickProp(0, _config.StationProps.size() - 1);
    _stations.reserve(_config.StationCount);

    for (int index = 0; index < _config.StationCount; ++index) {
        glm::vec3 position{0.0f};
        for (int attempt = 0; attempt < 64; ++attempt) {
            const float radius = _config.MapRadius * std::sqrt(RandomFloat(0.0f, 1.0f));
            const float angle = RandomFloat(0.0f, glm::two_pi<float>());
            position = glm::vec3(
                radius * std::cos(angle),
                RandomFloat(_config.AltitudeMin, _config.AltitudeMax),
                radius * std::sin(angle)
            );

            bool tooClose = false;
            for (const auto& station : _stations) {
                if (glm::length(station.Position - position) < _config.MinSeparation) {
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) break;
        }

        auto prop = _assets.GetProp(_config.StationProps[pickProp(_rng)]).lock();
        be_assert(prop, "DeliverySystem: missing station prop");

        const auto entity = CreateEntity(_registry
            ,NameComponent { .Name = "station-" + std::to_string(index) }
            ,TransformComponent { .Position = position, .Scale = glm::vec3(_config.StationScale) }
            ,RenderComponent { .Prop = prop, .CastShadows = false }
            ,StationComponent { .Index = index }
        );
        _stations.push_back({ .Position = position, .Entity = entity });
    }
}

auto DeliverySystem::Begin(glm::vec3 shipPos) -> void {
    if (_stations.empty()) return;
    _target = PickTargetExcept(NearestStation(shipPos));
}

auto DeliverySystem::Update(glm::vec3 shipPos) -> bool {
    if (_target < 0) return false;
    if (glm::length(_stations[_target].Position - shipPos) > _config.VisitRadius) return false;

    ++_delivered;
    _target = PickTargetExcept(_target);
    return true;
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
