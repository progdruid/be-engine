#pragma once

#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include <umbrellas/common.hpp>
#include <umbrellas/include-glm.h>
#include <umbrellas/include-libassert.h>

struct FloatRange {
    float Min;
    float Max;

    template <typename Rng>
    auto Pick(Rng& rng) const -> float {
        return std::uniform_real_distribution<float>(Min, Max)(rng);
    }
};

struct StationKind {
    std::string Prop;
    std::filesystem::path Path;
    float EmissiveMix { 0.5f };
    glm::vec3 AimPoint { 0.0f, 0.0f, 0.0f };
    glm::vec3 Origin { 0.0f, 0.0f, 0.0f };
    float Scale { 1.f };
    bool Flying { false };
    FloatRange AltitudeRange { 0.0f, 0.0f };
    FloatRange RotationX { 0.0f, glm::two_pi<float>() };
    FloatRange RotationY { 0.0f, glm::two_pi<float>() };
    FloatRange RotationZ { 0.0f, glm::two_pi<float>() };
    float DockRadius { 5.0f };
    std::vector<glm::vec3> DockPositions {};
};

struct Commodity {
    std::string Name;
    float AverageValue = 0.0f;
};

struct RiftSettings {
    unsigned Seed = 1337;

    struct CameraSettings {
        float NearPlane = 0.5f;
        float FarPlane = 300.0f;
        float SpawnHeight = 100.0f;
    } Camera;

    struct AmbientSettings {
        //glm::vec3 Color = HexColor("#0A122E");
        //glm::vec3 Color = HexColor("#222222");
        //glm::vec3 Color = HexColor("#FF7F11");
        glm::vec3 Color = HexColor("#000000");
        //glm::vec3 Color = HexColor("#2E4372");
    } Ambient;

    struct TerrainSettings {
        float Size = 180.0f;
        int Cells = 45;
        float MapSize = 2880.0f;
        float SpikeAmplitude = 75.0f;
        int ValleyFrequency = 8;
        float ValleyWidth = 0.05f;
        float WallSlope = 0.10f;
        float HighlandBase = 30.0f;
        float ValleyRoughness = 24.0f;
        glm::vec3 Color = HexColor("#F28123");
    } Terrain;

    struct PosterizeSettings {
        float PixelSize = 4.0f;
        float DitherSpread = 1.0f;
        float FogStart = 100.0f;
        float FogEnd = 200.0f;
        glm::vec3 FogColor = HexColor("#2E4372");
        bool Enabled = true;
        std::vector<glm::vec3> Palette = {
            HexColor("#2E4372"),
            HexColor("#E89128"),
            HexColor("#FFFFFF"),
            HexColor("#D34E24"),
            HexColor("#8C3318"),
            HexColor("#1F2C47"),
        };
    } Posterize;

    struct SunSettings {
        glm::vec3 Direction = { -0.5f, -1.0f, -0.5f };
        glm::vec3 Color = glm::vec3(1.0f);
        float Power = 1.0f;
        float ShadowCameraDistance = 50.0f;
        float ShadowMapWorldSize = 30.0f;
        float ShadowNearPlane = 0.1f;
        float ShadowFarPlane = 200.0f;
    } Sun;

    struct ShipSettings {
        float PitchRate = 90.0f;
        float YawRate = 70.0f;
        float RollRate = 120.0f;
        float MouseSensitivity = 1.0f;
        float AimRadius = 150.0f;
        float AimDeadZone = 0.06f;
        float MouseReturn = 0.0f;
        float RotationResponse = 6.0f;
        float RollAccel = 2.0f;
        float RollDecel = 4.0f;
        bool InvertPitch = false;

        float ThrustAccel = 40.0f;
        float BoostMultiplier = 3.0f;
        float MaxSpeed = 80.0f;
        float FlightAssistDamping = 2.0f;
        float FullStopDamping = 6.0f;
        bool FlightAssist = true;

        float GroundEffectLowAltitude = 20.0f;
        float GroundEffectHighAltitude = 40.0f;
        float GroundEffectSpeedLow = 1.0f;
        float GroundEffectDragLow = 1.0f;
        float GroundEffectSpeedHigh = 0.7f;
        float GroundEffectDragHigh = 1.0f;
        float GroundEffectResponse = 2.0f;

        float CollisionRadius = 2.0f;
        float GroundFriction = 3.0f;
        float CrashImpactSpeed = 25.0f;
        float DeathFadeOutTime = 0.5f;
        float DeathHoldTime = 1.0f;
        float DeathFadeInTime = 0.3f;

        float DockSpringFrequency = 8.0f;
        float DockDampingRatio = 0.3f;
    } Ship;

    struct DeliverySettings {
        std::vector<StationKind> Kinds = {
            //{
            //    .Prop = "station-solar-1",
            //    .Path = "assets/rift/simplhme_solar_station_1.glb",
            //    .EmissiveMix = 0.5f,
            //    .AimPoint = glm::vec3(0.0f, 3.0f, 0.0f),
            //    .Scale = 10.0f,
            //    .Flying = false,
            //},
            //{
            //    .Prop = "station-solar-2",
            //    .Path = "assets/rift/simple_solar_station_2.glb",
            //    .EmissiveMix = 0.5f,
            //    .AimPoint = glm::vec3(0.0f, 3.0f, 0.0f),
            //    .Scale = 8.0f,
            //    .Flying = false,
            //},
            //{
            //    .Prop = "station-solar-3",
            //    .Path = "assets/rift/simple_solar_station_3.glb",
            //    .EmissiveMix = 0.5f,
            //    .AimPoint = glm::vec3(0.0f, 3.0f, 0.0f),
            //    .Scale = 8.0f,
            //    .Flying = false,
            //},
            //{
            //    .Prop = "flying-station",
            //    .Path = "assets/rift/flying_station.glb",
            //    .EmissiveMix = 0.5f,
            //    .AimPoint = glm::vec3(0.0f, 1.0f, 0.0f),
            //    .Scale = 40.0f,
            //    .Flying = true,
            //},
            {
                .Prop = "oxygen-station",
                .Path = "assets/rift/oxygen_station.glb",
                .EmissiveMix = 0.5f,
                .AimPoint = glm::vec3(0.0f, 0.0f, 0.0f),
                .Origin = glm::vec3(0.0f, -2.5f, 0.0f),
                .Scale = 30.0f,
                .Flying = false,
                .RotationX = {0.f, 0.f },
                .RotationZ = {0.f, 0.f },
                .DockRadius = 7.0f,
                .DockPositions = {
                    glm::vec3(0.43f, 0.8f,  0.86f),
                    glm::vec3(0.43f, 0.8f, -0.06f),
                },
            },
            //{
            //    .Prop = "mining-station",
            //    .Path = "assets/rift/mining_station/scene.gltf",
            //    .EmissiveMix = 0.5f,
            //    .AimPoint = glm::vec3(0.0f, 3.0f, 0.0f),
            //    .Scale = 0.01f,
            //    .Flying = true,
            //},
            {
                .Prop = "iss",
                .Path = "assets/rift/iss/scene.gltf",
                .EmissiveMix = 0.5f,
                .AimPoint = glm::vec3(0.0f, 0.0f, 0.0f),
                .Scale = 80.0f,
                .Flying = true,
                .AltitudeRange = { 200.0f, 350.0f },
                .DockRadius = 10.0f,
                .DockPositions = {
                    glm::vec3( 0.0f ,  0.0f  ,  1.85f ),
                    glm::vec3( 0.0f , -1.25f , -0.514f),
                    glm::vec3(-0.22f, -0.905f, -0.514f),
                    glm::vec3( 0.0f ,  0.0f  , -1.45f )
                },
            },
            //{
            //    .Prop = "power_station",
            //    .Path = "assets/rift/power_station/scene.gltf",
            //    .EmissiveMix = 0.5f,
            //    .AimPoint = glm::vec3(0.0f, 3.0f, 0.0f),
            //    .Scale = 2.0f,
            //    .Flying = true,
            //},
        };
        int StationCount = 12;
        int JobsPerStation = 4;
        float RewardBase = 50.0f;
        float RewardPerMeter = 0.2f;
        float MapRadius = 2500.0f;
        float MinSeparation = 250.0f;
        float VisitRadius = 35.0f;

        std::vector<Commodity> Commodities = {
            { .Name = "Water Ice",   .AverageValue = 20.0f },
            { .Name = "Iron Ore",    .AverageValue = 35.0f },
            { .Name = "Oxygen",      .AverageValue = 55.0f },
            { .Name = "Silicon",     .AverageValue = 70.0f },
            { .Name = "Titanium",    .AverageValue = 130.0f },
            { .Name = "Rare Earths", .AverageValue = 220.0f },
            { .Name = "Helium-3",    .AverageValue = 340.0f },
            { .Name = "Antimatter",  .AverageValue = 820.0f },
        };
        int CargoCapacity = 40;
        float PriceDeviation = 0.28f;
        int MarketMinCommodities = 2;
        int MarketMaxCommodities = 5;

        struct MarkerSettings {
            float ScreenMargin = 0.88f;
            float MinRadius = 1.5f;
            float MaxRadius = 10.0f;
            float SizeFar = 800.0f;
            float SizeNear = 120.0f;
            float FadeNear = 80.0f;
            float FadeFar = 200.0f;
        } Marker;
    } Delivery;
};

class RiftStore {
    hide
    inline static RiftSettings* _settings = nullptr;

    expose
    static auto Bootstrap() -> void {
        be_assert(_settings == nullptr, "RiftStore already bootstrapped");
        _settings = new RiftSettings();
    }

    static auto Shutdown() -> void {
        delete _settings;
        _settings = nullptr;
    }

    static auto Get() -> RiftSettings& {
        be_assert(_settings != nullptr, "RiftStore accessed before Bootstrap");
        return *_settings;
    }
};
