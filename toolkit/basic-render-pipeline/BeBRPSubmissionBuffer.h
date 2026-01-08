#pragma once
#include <memory>
#include <vector>
#include <umbrellas/access-modifiers.hpp>
#include <umbrellas/include-glm.h>

class BeTexture;
struct BeModel;

struct BeBRPGeometryEntry {
    glm::vec3 Position = {0.f, 0.f, 0.f};
    glm::quat Rotation = glm::quat(glm::vec3(0, 0, 0));
    glm::vec3 Scale = {1.f, 1.f, 1.f};
    std::shared_ptr<BeModel> Model = nullptr;
    bool CastShadows = true;
};

struct BeBRPSunLightEntry {
    // Light properties (for lighting pass)
    glm::vec3 Direction;
    glm::vec3 Color;
    float Power;

    // Shadow map properties (for shadow pass)
    bool CastsShadows = true;
    uint32_t ShadowMapResolution;
    float ShadowCameraDistance;
    float ShadowMapWorldSize;
    float ShadowNearPlane;
    float ShadowFarPlane;
    std::weak_ptr<BeTexture> ShadowMap;

    glm::mat4 ViewProjection;

    auto CalculateMatrix() -> void {
        const float halfSize = ShadowMapWorldSize * 0.5f;
        const glm::mat4 lightOrtho = glm::orthoLH_ZO(-halfSize, halfSize, -halfSize, halfSize, ShadowNearPlane, ShadowFarPlane);
        const glm::vec3 lightPos = -Direction * ShadowCameraDistance;
        const glm::mat4 lightView = glm::lookAtLH(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ViewProjection = lightOrtho * lightView;
    }
};

struct BeBRPPointLightEntry {
    expose
    glm::vec3 Position;
    float Radius;
    glm::vec3 Color;
    float Power;

    bool CastsShadows = false;
    uint32_t ShadowMapResolution = 1024;
    float ShadowNearPlane = 0.1f; // far plane is radius
    std::weak_ptr<BeTexture> ShadowMap; 
};

class BeBRPSubmissionBuffer {
    
    hide
    std::vector<BeBRPGeometryEntry> _geometryEntries;
    std::vector<BeBRPSunLightEntry> _sunLightEntries;
    std::vector<BeBRPPointLightEntry> _pointLightEntries;
    
    expose
    explicit BeBRPSubmissionBuffer() = default;
    ~BeBRPSubmissionBuffer() = default;
    
    expose
    auto ClearEntries () -> void;
    auto SubmitGeometry (const BeBRPGeometryEntry& entry) -> void;
    auto SubmitSunLight(const BeBRPSunLightEntry& entry) -> void;
    auto SubmitPointLight(const BeBRPPointLightEntry& entry) -> void;
    
    expose
    auto GetGeometryEntries () const -> const std::vector<BeBRPGeometryEntry>&;
    auto GetSunLightEntries () const -> const std::vector<BeBRPSunLightEntry>&;
    auto GetPointLightEntries () const -> const std::vector<BeBRPPointLightEntry>&;
};
