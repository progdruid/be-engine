#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>

#include "entt/entt.hpp"

class BeAssetRegistry;
class RiftTerrain;

struct StationComponent {
    int Index = 0;
};

struct DockComponent {
    int StationIndex = -1;
};

class DeliverySystem {
    expose
    struct Job {
        int Destination = -1;
        float Distance = 0.0f;
        int Reward = 0;
    };

    expose
    struct MarketEntry {
        int Commodity = -1;
        int Price = 0;
        bool ForSale = false;
    };

    hide
    struct Station {
        glm::vec3 Position;
        glm::vec3 Aim;
        entt::entity Entity;
        float DockRadius = 0.f;
        std::string Name;
        std::vector<glm::vec3> Docks;
        std::vector<Job> Jobs;
        std::vector<MarketEntry> Market;
    };

    entt::registry& _registry;
    BeAssetRegistry& _assets;
    const RiftTerrain& _terrain;
    std::mt19937 _rng;
    std::vector<Station> _stations;
    std::vector<int> _cargo;
    std::optional<Job> _contract;
    int _dockedStation = -1;
    int _credits = 0;

    expose
    DeliverySystem(entt::registry& registry, BeAssetRegistry& assets, const RiftTerrain& terrain);
    ~DeliverySystem();

    expose
    struct DockHit {
        bool Hit = false;
        glm::vec3 Anchor{0.0f};
        int Station = -1;
    };

    auto GenerateStations() -> void;
    auto CheckDock(glm::vec3 shipPos) const -> DockHit;
    auto NotifyDocked(const DockHit& hit) -> void;
    auto NotifyUndocked() -> void;

    auto TakeJob(int station, int jobIndex) -> void;
    auto CompleteContract() -> void;
    auto ApplyCrashPenalty() -> void;

    auto BuyCommodity(int station, int commodity, int tons) -> void;
    auto SellCommodity(int station, int commodity, int tons) -> void;
    [[nodiscard]] auto GetRespawnDock(glm::vec3 shipPos) -> glm::vec3;

    [[nodiscard]] auto HasContract() const -> bool { return _contract.has_value(); }
    [[nodiscard]] auto CanComplete() const -> bool { return _contract && _dockedStation >= 0 && _dockedStation == _contract->Destination; }
    [[nodiscard]] auto GetTargetPosition(glm::vec3 shipPos) const -> glm::vec3;
    [[nodiscard]] auto GetStationCount() const -> int { return static_cast<int>(_stations.size()); }
    [[nodiscard]] auto GetDistanceToTarget(glm::vec3 shipPos) const -> float;
    [[nodiscard]] auto GetDockedStation() const -> int { return _dockedStation; }
    [[nodiscard]] auto GetCredits() const -> int { return _credits; }
    [[nodiscard]] auto GetContract() const -> const std::optional<Job>& { return _contract; }
    [[nodiscard]] auto GetStationJobs(int station) const -> const std::vector<Job>& { return _stations[station].Jobs; }
    [[nodiscard]] auto GetStationName(int station) const -> std::string;

    [[nodiscard]] auto GetStationMarket(int station) const -> const std::vector<MarketEntry>& { return _stations[station].Market; }
    [[nodiscard]] auto GetCommodityName(int commodity) const -> std::string;
    [[nodiscard]] auto GetCommodityAverage(int commodity) const -> float;
    [[nodiscard]] auto GetCargoTons(int commodity) const -> int { return _cargo[commodity]; }
    [[nodiscard]] auto GetCargoUsed() const -> int;
    [[nodiscard]] auto GetCargoCapacity() const -> int;
};
