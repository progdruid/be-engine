#pragma once

#include <random>
#include <string>
#include <vector>

#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "entt/entt.hpp"

class BeAssetRegistry;

struct StationComponent {
    int Index = 0;
};

class DeliverySystem {
    expose
    struct Config {
        std::vector<std::string> StationProps;
        int StationCount;
        float MapRadius;
        float AltitudeMin;
        float AltitudeMax;
        float MinSeparation;
        float VisitRadius;
        float StationScale;
        unsigned Seed = 1337;
    };

    hide
    struct Station {
        glm::vec3 Position;
        entt::entity Entity;
    };

    entt::registry& _registry;
    BeAssetRegistry& _assets;
    Config _config;
    std::mt19937 _rng;
    std::vector<Station> _stations;
    int _target = -1;
    int _delivered = 0;

    expose
    explicit DeliverySystem(entt::registry& registry, BeAssetRegistry& assets, Config config);
    ~DeliverySystem();

    expose
    auto GenerateStations() -> void;
    auto Begin(glm::vec3 shipPos) -> void;
    auto Update(glm::vec3 shipPos) -> bool;

    [[nodiscard]] auto HasTarget() const -> bool { return _target >= 0; }
    [[nodiscard]] auto TargetPosition() const -> glm::vec3 { return _stations[_target].Position; }
    [[nodiscard]] auto DeliveredCount() const -> int { return _delivered; }
    [[nodiscard]] auto StationCount() const -> int { return static_cast<int>(_stations.size()); }
    [[nodiscard]] auto DistanceToTarget(glm::vec3 shipPos) const -> float;

    hide
    auto NearestStation(glm::vec3 pos) const -> int;
    auto PickTargetExcept(int except) -> int;
    auto RandomFloat(float lo, float hi) -> float;
};
