#pragma once

#include <random>
#include <vector>

#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "entt/entt.hpp"

class BeAssetRegistry;

struct StationComponent {
    int Index = 0;
};

struct DockComponent {
    int StationIndex = -1;
};

class DeliverySystem {
    hide
    struct Station {
        glm::vec3 Position;
        glm::vec3 Aim;
        entt::entity Entity;
        float DockRadius = 0.f;
        std::vector<glm::vec3> Docks;
    };

    entt::registry& _registry;
    BeAssetRegistry& _assets;
    std::mt19937 _rng;
    std::vector<Station> _stations;
    int _target = -1;
    int _delivered = 0;

    expose
    explicit DeliverySystem(entt::registry& registry, BeAssetRegistry& assets);
    ~DeliverySystem();

    expose
    struct DockHit {
        bool Hit = false;
        glm::vec3 Anchor{0.0f};
        bool IsTarget = false;
    };

    auto GenerateStations() -> void;
    auto Begin(glm::vec3 shipPos) -> void;
    auto CheckDock(glm::vec3 shipPos) const -> DockHit;
    auto NotifyDocked(const DockHit& hit) -> void;
    auto TargetNearest(glm::vec3 shipPos) -> void;

    [[nodiscard]] auto HasTarget() const -> bool { return _target >= 0; }
    [[nodiscard]] auto TargetPosition(glm::vec3 shipPos) const -> glm::vec3;
    [[nodiscard]] auto DeliveredCount() const -> int { return _delivered; }
    [[nodiscard]] auto StationCount() const -> int { return static_cast<int>(_stations.size()); }
    [[nodiscard]] auto DistanceToTarget(glm::vec3 shipPos) const -> float;

    hide
    auto NearestStation(glm::vec3 pos) const -> int;
    auto PickTargetExcept(int except) -> int;
};
