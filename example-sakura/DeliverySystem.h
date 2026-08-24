#pragma once

#include <filesystem>
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
    struct StationKind {
        std::string Prop;
        std::filesystem::path Path;
        float EmissiveMix;
        glm::vec3 AimPoint;
        glm::vec3 Origin;
        float Scale;
        bool Flying;
    };

    struct Config {
        std::vector<StationKind> StationKinds;
        int StationCount;
        float MapRadius;
        float AltitudeMin;
        float AltitudeMax;
        float MinSeparation;
        float VisitRadius;
        float TerrainSize;
        float TerrainSpikeAmplitude;
        unsigned Seed = 1337;
    };

    hide
    struct Station {
        glm::vec3 Position;
        glm::vec3 Aim;
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
    auto TargetNearest(glm::vec3 shipPos) -> void;

    [[nodiscard]] auto HasTarget() const -> bool { return _target >= 0; }
    [[nodiscard]] auto TargetPosition() const -> glm::vec3 { return _stations[_target].Aim; }
    [[nodiscard]] auto DeliveredCount() const -> int { return _delivered; }
    [[nodiscard]] auto StationCount() const -> int { return static_cast<int>(_stations.size()); }
    [[nodiscard]] auto DistanceToTarget(glm::vec3 shipPos) const -> float;

    hide
    auto NearestStation(glm::vec3 pos) const -> int;
    auto PickTargetExcept(int except) -> int;
    auto RandomFloat(float lo, float hi) -> float;
};
